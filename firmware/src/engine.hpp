#pragma once
#include "core/button_input.hpp"
#include "nfc/ndef_parser.hpp"
#include "protocol.hpp"
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace okanegachi {
enum class Load { Missing, Ok, Corrupt };
struct Store {
  virtual ~Store() = default;
  virtual Load read(std::string &) = 0;
  virtual bool write(const std::string &) = 0;
};
struct Clock {
  virtual ~Clock() = default;
  virtual uint64_t now() const = 0;
  virtual int64_t utc() const = 0;
};
struct Random {
  virtual ~Random() = default;
  virtual std::string uuid() = 0;
  virtual uint32_t jitter(uint32_t maximum) = 0;
};
struct Trace {
  std::string type, detail, id;
  int seq{};
};
struct Request {
  std::string id, body;
};
enum class Page { Pet, Goal, CheckIn, Menu, Review, Game, Connection };
enum class Observation { Present, Absent, Error };
struct Game {
  bool started{}, paused{};
  int lane{1}, star_lane{}, score{}, dropped{};
  uint64_t played_ms{}, last_tick{};
};
struct Persistent {
  std::string binding, epoch, cached_response;
  int ack{}, version{};
  bool mute{};
  std::vector<Event> outbox;
};
std::string encode_persistent(const Persistent &p);
std::optional<Persistent> decode_persistent(const std::string &data);
class Engine {
public:
  Engine(Store &store, Clock &clock, Random &random, std::string device,
         std::string binding);
  bool begin();
  void tick();
  void button(core::ButtonId id, bool down) { buttons_.set_raw(id, down); }
  void boot_button(core::ButtonId id, bool down) {
    buttons_.set_held_on_boot(id, down);
  }
  void connectivity(bool connected);
  std::optional<Request> next_request(int rssi = -127);
  bool complete(const std::string &request_id, int status,
                const std::string &body, uint64_t retry_after_ms = 0);
  void nfc(Observation observation,
           nfc::ParseResult parsed = nfc::ParseResult::Unrelated);
  bool enqueue(const Event &event);
  bool review(const char *source);
  void manual_retry();
  const State &state() const { return state_; }
  const Appearance &appearance() const { return visible_appearance_; }
  const Persistent &persistent() const { return saved_; }
  const std::optional<Review> &displayed_review() const {
    return displayed_review_;
  }
  Page page() const { return page_; }
  const Game &game() const { return game_; }
  std::string animation() const;
  uint64_t animation_elapsed() const;
  const std::string &status() const { return status_; }
  const std::string &notice() const { return notice_; }
  bool online() const { return connected_; }
  bool storage_fault() const { return storage_fault_; }
  bool in_flight() const { return in_flight_; }
  bool demo_fresh() const;
  size_t queued() const { return pending_.size(); }
  int menu_index() const { return menu_index_; }
  uint64_t last_sync() const { return last_success_; }
  int take_tone() {
    int n = tone_;
    tone_ = 0;
    return saved_.mute ? 0 : n;
  }
  std::function<void(const Trace &)> trace;

private:
  Store &store_;
  Clock &clock_;
  Random &random_;
  std::string device_, binding_;
  Persistent saved_;
  State state_;
  Appearance visible_appearance_;
  core::ButtonInput buttons_;
  Page page_{Page::Pet};
  Game game_;
  int menu_index_{};
  std::optional<Review> displayed_review_;
  std::vector<Command> pending_;
  std::optional<Command> active_;
  uint64_t active_start_{}, local_start_{}, local_end_{};
  std::string local_animation_;
  std::string status_{"offline"}, notice_{"Waiting for setup"};
  bool connected_{}, storage_fault_{}, in_flight_{}, dirty_{true}, blocked_{},
      have_success_{}, demo_{};
  uint64_t next_poll_{}, not_before_{}, last_send_{}, last_success_{},
      anchor_mono_{};
  int64_t anchor_utc_{};
  bool sent_before_{};
  unsigned failures_{};
  int tone_{};
  Request request_;
  std::vector<std::string> submitted_;
  bool nfc_armed_{true}, absence_started_{};
  uint64_t absence_start_{};
  std::string intent_;
  uint64_t intent_deadline_{};
  void emit(std::string type, std::string detail = {}, std::string id = {},
            int seq = 0);
  bool commit(const Persistent &candidate);
  void backoff(const std::string &reason, uint64_t minimum = 0);
  void start_next();
  bool consume(int seq);
  int64_t utc_now() const;
  void on_button(const core::ButtonTrace &event);
  void navigate(int direction);
  void select();
  void confirm_review();
  void start_game();
  bool action(const char *type, const std::string &payload);
};
} // namespace okanegachi
