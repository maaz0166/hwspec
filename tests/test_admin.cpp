#include "admin.hpp"
#include "args.hpp"
#include "run.hpp"
#include "check.hpp"
#include "hwspec/format.hpp"
#include "hwspec/json.hpp"
#include <sstream>

using namespace hwspec;
using namespace hwspec::cli;
namespace {
bool has(const std::string& value, const std::string& part) { return value.find(part) != std::string::npos; }
struct Fixture {
  int prompts=0, launches=0, pauses=0, reads=0, privileged_reads=0;
  char choice='1';
  bool launched=true;
  std::vector<std::string> child_args;
  AdminRuntime runtime;
  std::vector<Section> sections;
  std::ostringstream out, err;
  Fixture() {
    runtime.prompt=[this](const std::vector<std::string>& titles) {
      ++prompts; CHECK_EQ(titles.size(),std::size_t(1)); CHECK_EQ(titles[0],std::string("disk health"));
      return choice;
    };
    runtime.relaunch=[this](const std::vector<std::string>& args) { ++launches; child_args=args; return launched; };
    runtime.pause=[this] { ++pauses; };
    sections={
      {"cpu","CPU",false,[this] { ++reads; return SectionReport{"CPU",{{"Name","Fake"}}}; },{},{}},
      {"disk","Disk",false,[this] { ++reads; return SectionReport{"Disk",{{"Drive","C:"}}}; },{},{},
       true,"disk health",
       [this](bool allow) { ++reads; if(allow)++privileged_reads;
         return SectionReport{"Disk",{{"Drive","C:"},{"Disk health",allow?"Unknown":"requires admin"}}}; },
       [this](bool detail,bool allow) { ++reads; if(detail && allow)++privileged_reads;
         return detail?(allow?"{\"disk_health\":{\"status\":\"unknown\"}}":
                            "{\"disk_health\":{\"status\":\"requires_admin\"}}"):
                       "{\"drives\":[]}"; }}
    };
  }
  int run_args(const std::vector<std::string>& args) { return run(args,out,err,sections,runtime); }
};
void decision_table() {
  for(bool elevated:{false,true})for(bool interactive:{false,true})
    for(bool json:{false,true})for(bool skip:{false,true}) {
      Fixture f;
      f.runtime.elevated=elevated; f.runtime.interactive=interactive;
      std::vector<std::string> args;
      args.reserve(4);
      args.emplace_back("--disk");
      args.emplace_back("-d");
      if(json)args.emplace_back("--json");
      if(skip)args.emplace_back("--skip-admin");
      CHECK_EQ(f.run_args(args),0);
      CHECK_EQ(f.prompts,(!elevated && interactive && !json && !skip)?1:0);
      CHECK_EQ(f.privileged_reads,(elevated && !skip)?1:0);
      CHECK_EQ(f.reads,1); CHECK_EQ(f.launches,0); CHECK(f.err.str().empty());
      CHECK(has(f.out.str(),elevated && !skip?(json?"unknown":"Unknown"):(json?"requires_admin":"requires admin")));
    }
}
void no_privilege_needed() {
  for(const auto& args:std::vector<std::vector<std::string>>{{"--disk"},{"--cpu","-d"},{"--cpu","--json"}}) {
    Fixture f; f.runtime.interactive=true;
    CHECK_EQ(f.run_args(args),0); CHECK_EQ(f.prompts,0); CHECK_EQ(f.privileged_reads,0);
  }
  Fixture all; all.runtime.interactive=true;
  CHECK_EQ(all.run_args({"--all","-d"}),0); CHECK_EQ(all.prompts,1); CHECK_EQ(all.reads,2);
}
void choices() {
  Fixture cancel; cancel.runtime.interactive=true; cancel.choice='3';
  CHECK_EQ(cancel.run_args({"--disk","-d"}),1); CHECK_EQ(cancel.reads,0); CHECK(cancel.out.str().empty());
  Fixture reopen; reopen.runtime.interactive=true; reopen.choice='2';
  CHECK_EQ(reopen.run_args({"--disk","-d"}),0); CHECK_EQ(reopen.launches,1); CHECK_EQ(reopen.reads,0);
  CHECK(reopen.child_args==std::vector<std::string>({"--disk","-d","--pause"}));
  Fixture decline; decline.runtime.interactive=true; decline.choice='2'; decline.launched=false;
  CHECK_EQ(decline.run_args({"--disk","-d"}),0); CHECK_EQ(decline.privileged_reads,0);
  CHECK(has(decline.out.str(),"requires admin")); CHECK(has(decline.err.str(),"elevation declined"));
}
void explicit_elevation() {
  Fixture f;
  CHECK_EQ(f.run_args({"--json","--disk","-d","--elevate","--pause"}),0);
  CHECK_EQ(f.launches,1); CHECK_EQ(f.prompts,0); CHECK_EQ(f.reads,0);
  CHECK(f.child_args==std::vector<std::string>({"--json","--disk","-d","--pause"}));
  Fixture elevated; elevated.runtime.elevated=true;
  CHECK_EQ(elevated.run_args({"--disk","-d","--elevate"}),0);
  CHECK_EQ(elevated.launches,0); CHECK_EQ(elevated.privileged_reads,1);
  Fixture unavailable; unavailable.runtime.relaunch={};
  CHECK_EQ(unavailable.run_args({"--disk","-d","--elevate"}),0);
  CHECK(has(unavailable.out.str(),"requires admin")); CHECK_EQ(unavailable.privileged_reads,0);
}
void pause_and_early_exit() {
  Fixture f; f.runtime.interactive=true;
  CHECK_EQ(f.run_args({"--cpu","--pause"}),0); CHECK_EQ(f.pauses,1);
  Fixture json; json.runtime.interactive=true;
  CHECK_EQ(json.run_args({"--cpu","--pause","--json"}),0); CHECK_EQ(json.pauses,0);
  Fixture piped;
  CHECK_EQ(piped.run_args({"--cpu","--pause"}),0); CHECK_EQ(piped.pauses,0);
  for(const auto& flag:{"--help","--version","--bogus"}) {
    Fixture early; early.runtime.interactive=true;
    early.run_args({flag,"--elevate","--pause"});
    CHECK_EQ(early.launches,0); CHECK_EQ(early.prompts,0); CHECK_EQ(early.pauses,0); CHECK_EQ(early.reads,0);
  }
}
void whole_section_requires_admin() {
  Fixture f;
  f.sections[0].requires_admin=true;
  CHECK_EQ(f.run_args({"--cpu","--json"}),0);
  CHECK_EQ(f.out.str(),std::string("{\"cpu\":{\"status\":\"requires_admin\"}}\n")); CHECK_EQ(f.reads,0);
  Fixture text; text.sections[0].requires_admin=true;
  CHECK_EQ(text.run_args({"--cpu"}),0); CHECK(has(text.out.str(),"requires admin")); CHECK_EQ(text.reads,0);
}
void smart_output() {
  DiskInfo disk;
  CHECK(has(json_disk(disk,true),"requires_admin"));
  CHECK(has(render_text({format_disk(disk,true)}),"requires admin")); // even with no logical drives
  CHECK(!has(json_disk(disk,false),"disk_health"));
  disk.health_status=DiskHealthStatus::unknown;
  CHECK(has(json_disk(disk,true),"\"status\":\"unknown\""));
  CHECK(has(render_text({format_disk(disk,true)}),"Unknown"));
  disk.health_status=DiskHealthStatus::available;
  disk.health_devices={{std::string("Device A"),false},{std::string("Device B"),true},{std::nullopt,std::nullopt}};
  const auto json=json_disk(disk,true);
  CHECK(has(json,"\"failure_predicted\":false")); CHECK(has(json,"\"failure_predicted\":true"));
  CHECK(has(json,"\"failure_predicted\":null")); CHECK(has(json,"\"instance_name\":null"));
  const auto text=render_text({format_disk(disk,true)});
  CHECK(has(text,"No failure predicted")); CHECK(has(text,"Failure predicted")); CHECK(has(text,"Unknown"));
}
void arguments() {
  const auto parsed=parse_args({"--skip-admin","--pause","--elevate"},{});
  CHECK(parsed.error.empty()); CHECK(parsed.skip_admin); CHECK(parsed.pause); CHECK(parsed.elevate);
  CHECK(elevated_arguments({"--elevate","--cpu","--pause","--pause"})==std::vector<std::string>({"--cpu","--pause"}));
  CHECK(quote_windows_argument(L"")==L"\"\"");
  CHECK(quote_windows_argument(L"a b")==L"\"a b\"");
  CHECK(quote_windows_argument(L"C:\\a b\\")==L"\"C:\\a b\\\\\"");
  CHECK(quote_windows_argument(L"a\"b")==L"\"a\\\"b\"");
}
} // namespace
int main() {
  decision_table(); no_privilege_needed(); choices(); explicit_elevation();
  pause_and_early_exit(); whole_section_requires_admin(); smart_output(); arguments();
  return finish();
}
