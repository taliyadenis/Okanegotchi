#include "fakes.hpp"
#include <cassert>
#include <iostream>
int main() {
    using namespace okanegachi::test;
    FakeClock clock;
    clock.monotonic_ms = 0xfffffff0ULL;
    const auto before = clock.millis32();
    clock.advance(40);
    assert(static_cast<std::uint32_t>(clock.millis32()-before) == 40);
    FakeStore store;
    assert(store.put("record", {1,2}));
    store.failures_remaining = 1;
    assert(!store.put("record", {3,4}));
    std::vector<std::uint8_t> bytes;
    assert(store.get("record", bytes) && bytes == std::vector<std::uint8_t>({1,2}));
    assert(store.attempts == 2 && store.successes == 1);
    FakeTransport transport;
    assert(transport.send("first"));
    assert(!transport.send("overlap"));
    transport.complete();
    assert(transport.send("second") && transport.request_bodies.size() == 2);
    std::cout << "PASS test-support smoke only; no firmware under test\n";
}
