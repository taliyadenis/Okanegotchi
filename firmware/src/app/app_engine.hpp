#pragma once
#include "app/fake_platform.hpp"
#include "core/button_input.hpp"
#include "core/demo_tag_router.hpp"
#include "core/nfc_presence.hpp"
#include "network/json_codec.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace okanegachi::app {

class AppEngine {
public:
  struct TraceEntry {
    std::map<std::string, std::string> fields;
    int seq{0};
  };

  void init_from_seed(const std::string& seed_json, const std::string& fixture_path);
  void reboot();
  std::string run_steps(const std::string& steps_json);

  std::string snapshot_json() const;
  std::string trace_json() const;
  std::string requests_json() const;

private:
  enum class Page { Pet, Goal, CheckIn, Menu, Review, Game };
  enum class NetStatus { Ready, Backoff, Unauthorized, Forbidden, UpdateRequired, Error };

  struct PendingCommand {
    int seq{0};
    std::string animation;
    int duration_ms{0};
    std::string expires_at;
  };

  FakeClock clock_;
  FakeStore store_;
  FakeServer server_;
  core::ButtonInput buttons_;
  core::NfcPresence nfc_;

  std::string device_id_{"cc159f6c-bdaf-4b78-8c93-e7a8e27620dc"};
  std::string epoch_;
  bool paired_{true};
  int ack_seq_{4};
  int state_version_{18};
  bool demo_mode_{true};
  int next_sync_ms_{2000};
  std::string care_stage_{"content"};
  bool ghost_seed_{false};
  bool storage_fault_{false};
  bool online_{true};
  bool mute_{false};
  std::int64_t last_demo_sync_ms_{0};

  std::vector<network::OutboxEvent> outbox_;
  std::vector<PendingCommand> command_queue_;
  std::optional<PendingCommand> active_command_;
  std::uint32_t active_anim_end_ms_{0};
  std::string active_animation_{"idle"};
  Page page_{Page::Pet};
  NetStatus net_{NetStatus::Ready};

  bool request_in_flight_{false};
  std::string pending_request_id_;
  std::vector<std::string> submitted_event_ids_;
  std::uint32_t next_sync_at_ms_{0};
  bool sync_due_{true};
  std::uint32_t last_request_ms_{0};
  bool reset_pending_{false};
  bool unauthorized_{false};

  std::optional<core::RouteDecision> pending_demo_intent_;
  std::uint32_t pending_intent_deadline_ms_{0};

  std::vector<TraceEntry> trace_;
  std::vector<std::string> requests_;

  void pump(std::uint64_t until_ms);
  void pump_once();
  void persist_state();
  void load_state();
  void emit_trace(const std::string& type, const std::map<std::string, std::string>& extra = {});
  bool save_cursor(int seq);
  void handle_buttons();
  void handle_nfc();
  void try_send_sync();
  void complete_http(int status, const std::string& body);
  void complete_sync(const std::string& patch_json);
  void apply_commands(const std::vector<network::Command>& commands, bool ghost);
  void apply_event_results(const std::vector<network::EventResult>& results);
  bool enqueue_event(const network::OutboxEvent& ev);
  std::string page_name() const;
  std::string net_name() const;
  bool demo_fresh() const;
  void process_step_json(const std::string& step_json);
};

} // namespace okanegachi::app
