#include "internals.h"

#include <assert.h>
#include <iomanip>
#include <locale>
#include <termios.h>

#define KEY_ESC 0x1b
#define KEY_DEL 0x7f
#define KEY_BELL 0x07

#define VT100_ERASE "\33[2K\r"
#define VT100_UP_1_LINE "\33[1A"
#define VT100_UP_2_LINES "\33[2A"
#define VT100_UP_3_LINES "\33[3A"

#define REPORT_PACKET_PERIOD 1000

namespace BDD {
namespace emulation {

class Reporter {
private:
  const meta_t &meta;

  uint64_t num_packets;
  bool warmup_mode;
  uint64_t packet_counter;
  time_ns_t time;

  uint64_t next_report;

public:
  Reporter(const meta_t &_meta, bool _warmup_mode)
      : meta(_meta), num_packets(0), warmup_mode(_warmup_mode),
        packet_counter(0), time(0), next_report(0) {
    struct termios term;
    tcgetattr(fileno(stdin), &term);

    term.c_lflag &= ~ECHO;
    tcsetattr(fileno(stdin), 0, &term);

    set_thousands_separator();
  }

  void set_num_packets(uint64_t _num_packets) {
    if (warmup_mode) {
      num_packets = 2 * _num_packets;
    } else {
      num_packets = _num_packets;
    }
  }

  void stop_warmup() { warmup_mode = false; }
  void inc_packet_counter() { packet_counter++; }
  void set_time(time_ns_t _time) { time = _time; }

  void set_thousands_separator() {
    struct my_numpunct : std::numpunct<char> {
    protected:
      virtual char do_thousands_sep() const override { return ','; }
      virtual std::string do_grouping() const override { return "\03"; }
    };

    auto my_locale = std::locale(std::cout.getloc(), new my_numpunct);
    std::cout.imbue(my_locale);
  }

  void show() {
    auto progress = (int)((100.0 * packet_counter) / num_packets);
    auto churn_fpm =
        (60.0 * meta.flows_expired) / (double)(meta.elapsed_time * 1e-9);
    auto percent_accepted = 100.0 * meta.accepted / meta.packet_counter;
    auto percent_rejected = 100.0 * meta.rejected / meta.packet_counter;

    if (packet_counter < next_report) {
      return;
    }

    if (next_report > 1) {
      for (auto i = 0; i < 13; i++)
        std::cout << VT100_UP_1_LINE;
    }

    std::cout << VT100_ERASE;
    std::cout << "Emulator data\n";

    std::cout << VT100_ERASE;
    std::cout << "  Progress      " << progress << " %\n";
    std::cout << VT100_ERASE;
    std::cout << "  Packets       " << packet_counter << "\n";
    std::cout << VT100_ERASE;
    std::cout << "  Total packets " << num_packets << "\n";
    std::cout << VT100_ERASE;
    std::cout << "  Time          " << time << " ns\n";
    std::cout << VT100_ERASE;
    std::cout << "  Warmup        " << warmup_mode << "\n";

    std::cout << VT100_ERASE;
    std::cout << "Metadata\n";
    std::cout << VT100_ERASE;
    std::cout << "  Packets       " << meta.packet_counter << "\n";
    std::cout << VT100_ERASE;
    std::cout << "  Accepted      " << meta.accepted;
    std::cout << " (" << percent_accepted << " %)\n";
    std::cout << VT100_ERASE;
    std::cout << "  Rejected      " << meta.rejected;
    std::cout << " (" << percent_rejected << "%)\n";
    std::cout << VT100_ERASE;
    std::cout << "  Elapsed       " << meta.elapsed_time << " ns\n";
    std::cout << VT100_ERASE;
    std::cout << "  Expired       " << meta.flows_expired;
    std::cout << " (" << churn_fpm << " fpm)\n";
    std::cout << VT100_ERASE;
    std::cout << "  Dchain allocs " << meta.dchain_allocations << "\n";

    fflush(stdout);

    next_report += REPORT_PACKET_PERIOD;
  }
};

} // namespace emulation
} // namespace BDD