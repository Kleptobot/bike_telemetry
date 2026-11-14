// TCX summary parser smoke test.
//
// Covers the crash/correctness regressions found in the LogSummaryScreen bug
// hunt:
//   * a LARGE, realistic TCX (the device's TCX output is by far its biggest
//     log format -- minutes of incremental parsing on the device) must parse
//     to completion through step() with no crash and no stall;
//   * lap-aggregate values must NOT be double-counted from trackpoint-level
//     <DistanceMeters>/<Calories>/<Cadence> (the old bug: distance came out
//     orders of magnitude too large on real rides);
//   * <TCXTrainingMetrics_TSS> (what TCXLogger actually writes) is read;
//   * adversarial inputs -- truncated, single-line, raw binary, empty, CRLF
//     -- all terminate without crashing;
//   * the LogSummaryScreen delete sequence (close parser, then remove) works.
//
// Build+run (from sim/):
//   g++ -Wall -std=gnu++17 -DARDUINO=10800 -Istubs -I../src
//       tests/TcxParser_smoke.cpp ../src/Loggers/LogSummary.cpp
//       ../src/HAL/SDCard.cpp stubs/Arduino.cpp stubs/SdFat.cpp
//       -o build/obike-tcx-smoke
//   ./build/obike-tcx-smoke tcx-smoke.dir
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "Loggers/LogSummary.hpp"
#include "HAL/SDCard.hpp"

static int failures = 0;
static void check(bool ok, const char* what) {
    printf("%s  %s\n", ok ? "  ok   " : "  FAIL ", what);
    if (!ok) failures++;
}

static void makeDir(const char* d) {
#ifdef _WIN32
    ::_mkdir(d);
#else
    ::mkdir(d, 0755);
#endif
}

// Drive the parser until step() reports EOF, the way LogSummaryScreen::update()
// does (it runs 16 steps per frame; the loop shape is equivalent here). The
// step guard catches the "parser never finishes" stall class of bug.
static bool driveToDone(LogParser& p) {
    long steps = 0;
    while (p.step()) {
        if (++steps > 2000000) return false;
    }
    return p.done();
}

static bool writeAll(SDCardSystem& sd, const char* path, const std::string& data) {
    File32 f = sd.openFile(path, O_WRITE | O_CREAT | O_TRUNC);
    if (!f) return false;
    size_t off = 0;
    while (off < data.size()) {
        size_t n = f.write((const uint8_t*)data.data() + off, data.size() - off);
        if (n == 0) break;
        off += n;
    }
    f.close();
    return off == data.size();
}

// One <Trackpoint> shaped exactly like TCXLogger::addTrackpoint writes them,
// including the three elements the parser must treat as trackpoint-local:
// cumulative <DistanceMeters>, per-sample <Calories>, instant <Cadence>.
static std::string trackpoint(int cumDistanceM) {
    char b[700];
    snprintf(b, sizeof(b),
             "          <Trackpoint>\n"
             "            <Time>2026-10-20T10:00:00</Time>\n"
             "            <Position>\n"
             "              <LatitudeDegrees>-37.800000</LatitudeDegrees>\n"
             "              <LongitudeDegrees>144.900000</LongitudeDegrees>\n"
             "            </Position>\n"
             "            <AltitudeMeters>42.00</AltitudeMeters>\n"
             "            <DistanceMeters>%d.0</DistanceMeters>\n"
             "            <Calories>0.15</Calories>\n"
             "            <HeartRateBpm>\n"
             "              <Value>141</Value>\n"
             "            </HeartRateBpm>\n"
             "            <Extensions>\n"
             "              <TPX xmlns=\"http://www.garmin.com/xmlschemas/TrackPointExtension/v1\">\n"
             "                <Power>200</Power>\n"
             "                <Cadence>86</Cadence>\n"
             "                <Speed>5.555</Speed>\n"
             "              </TPX>\n"
             "            </Extensions>\n"
             "          </Trackpoint>\n",
             cumDistanceM);
    return b;
}

static const char* kHead =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<TrainingCenterDatabase xmlns=\"http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2\">\n"
    "  <Activities>\n"
    "    <Activity Sport=\"Biking\">\n"
    "      <Id>2026-10-20T10:00:00</Id>\n"
    "      <Lap StartTime=\"2026-10-20T10:00:00\">\n"
    "        <TotalTimeSeconds>3600</TotalTimeSeconds>\n"
    "        <DistanceMeters>25000.5</DistanceMeters>\n"
    "        <MaximumSpeed>11.5</MaximumSpeed>\n"
    "        <Calories>600</Calories>\n"
    "        <AverageHeartRateBpm>\n"
    "          <Value>140</Value>\n"
    "        </AverageHeartRateBpm>\n"
    "        <MaximumHeartRateBpm>\n"
    "          <Value>175</Value>\n"
    "        </MaximumHeartRateBpm>\n"
    "        <Intensity>Active</Intensity>\n"
    "        <Cadence>85</Cadence>\n"
    "        <TriggerMethod>Manual</TriggerMethod>\n"
    "        <Extensions>\n"
    "          <LX xmlns=\"http://www.garmin.com/xmlschemas/ActivityExtension/v2\">\n"
    "            <AvgNormalizedPower>250</AvgNormalizedPower>\n"
    "            <IntensityFactor>0.750</IntensityFactor>\n"
    "            <TCXTrainingMetrics_TSS>60</TCXTrainingMetrics_TSS>\n"
    "          </LX>\n"
    "        </Extensions>\n"
    "        <Track>\n";

static const char* kTail =
    "        </Track>\n"
    "      </Lap>\n"
    "    </Activity>\n"
    "  </Activities>\n"
    "</TrainingCenterDatabase>\n";

int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    const char* card = (argc > 1) ? argv[1] : "tcx-smoke.dir";
    makeDir(card);
    simSetSdRoot(card);

    SDCardSystem sd;
    if (!sd.init(nullptr)) { printf("sd init failed\n"); return 2; }

    // ---- Case 1: large well-formed one-lap TCX ----
    // 2000 trackpoints ~= 1 MB -- minutes of parsing on the device, seconds
    // here. Trackpoint distances run 10..20000 m cumulative; the old parser
    // summed them into ~200 km of extra bogus distance.
    const char* p1 = "/ride1.tcx";
    {
        std::string doc = kHead;
        for (int i = 1; i <= 2000; i++) doc += trackpoint(i * 10);
        doc += kTail;
        check(writeAll(sd, p1, doc), "write ride1.tcx (~1 MB)");
    }
    {
        TcxLogParser p;
        check(p.open(&sd, p1), "ride1: parser opens");
        check(driveToDone(p), "ride1: parse terminates (no stall, no crash)");
        const RideStats& s = p.stats();
        check(s.valid, "ride1: stats valid");
        check(s.lapCount == 1, "ride1: lapCount == 1");
        check(s.durationSec == 3600, "ride1: duration == 3600 s");
        check(std::fabs(s.distanceM - 25000.5f) < 1.0f,
              "ride1: distance == lap header (NOT trackpoint-inflated)");
        check(std::fabs(s.calories - 600.0f) < 1.0f,
              "ride1: calories == lap header (NOT trackpoint-summed)");
        check(std::fabs(s.avgCadence - 85.0f) < 0.51f,
              "ride1: avgCadence == lap header (NOT trackpoint cadence)");
        check(std::fabs(s.avgHr - 140.0f) < 0.51f, "ride1: avgHR == lap <Value>");
        check(std::fabs(s.maxHr - 175.0f) < 0.51f, "ride1: maxHR == lap <Value>");
        check(std::fabs(s.maxSpeedKmh - 41.4f) < 0.1f,
              "ride1: maxSpeed = 11.5 m/s -> 41.4 km/h");
        check(std::fabs(s.np - 250.0f) < 0.5f, "ride1: NP == 250");
        check(std::fabs(s.tss - 60.0f) < 0.5f,
              "ride1: TSS == 60 (<TCXTrainingMetrics_TSS> matched)");
    }

    // ---- Case 2: two laps -- aggregates must SUM across laps ----
    const char* p2 = "/ride2.tcx";
    {
        std::string doc = kHead;
        doc += trackpoint(100);
        doc += "        </Track>\n"
               "      </Lap>\n"
               "      <Lap StartTime=\"2026-10-20T11:00:00\">\n"
               "        <TotalTimeSeconds>1800</TotalTimeSeconds>\n"
               "        <DistanceMeters>10000</DistanceMeters>\n"
               "        <MaximumSpeed>9.5</MaximumSpeed>\n"
               "        <Calories>300</Calories>\n"
               "        <AverageHeartRateBpm>\n"
               "          <Value>145</Value>\n"
               "        </AverageHeartRateBpm>\n"
               "        <MaximumHeartRateBpm>\n"
               "          <Value>170</Value>\n"
               "        </MaximumHeartRateBpm>\n"
               "        <Cadence>80</Cadence>\n"
               "        <Track>\n"
               "        </Track>\n"
               "      </Lap>\n"
               "    </Activity>\n"
               "  </Activities>\n"
               "</TrainingCenterDatabase>\n";
        check(writeAll(sd, p2, doc), "write ride2.tcx (2 laps)");
    }
    {
        TcxLogParser p;
        check(p.open(&sd, p2), "ride2: parser opens");
        check(driveToDone(p), "ride2: parse terminates");
        const RideStats& s = p.stats();
        check(s.lapCount == 2, "ride2: lapCount == 2");
        check(s.durationSec == 5400, "ride2: duration sums 3600+1800");
        check(std::fabs(s.distanceM - 35000.5f) < 1.0f, "ride2: distance sums");
        check(std::fabs(s.calories - 900.0f) < 1.0f, "ride2: calories sum");
        check(std::fabs(s.avgCadence - 82.5f) < 0.51f, "ride2: avgCadence (85+80)/2");
        check(std::fabs(s.avgHr - 142.5f) < 0.51f, "ride2: avgHR (140+145)/2");
    }

    // ---- Case 3: truncated mid-header ----
    const char* p3 = "/trunc.tcx";
    {
        std::string full = std::string(kHead) + trackpoint(10) + kTail;
        check(writeAll(sd, p3, full.substr(0, 400)), "write trunc.tcx");
    }
    {
        TcxLogParser p;
        check(p.open(&sd, p3), "trunc: parser opens");
        check(driveToDone(p), "trunc: parse terminates");
    }

    // ---- Case 4: one giant line, no newlines at all (the 256 B line buffer
    //      must cap, not overflow) ----
    const char* p4 = "/oneline.tcx";
    {
        std::string doc = std::string(kHead) + trackpoint(10) + kTail;
        for (char& c : doc) if (c == '\n') c = ' ';
        check(writeAll(sd, p4, doc), "write oneline.tcx");
    }
    {
        TcxLogParser p;
        check(p.open(&sd, p4), "oneline: parser opens");
        check(driveToDone(p), "oneline: parse terminates");
    }

    // ---- Case 5: raw binary garbage, newlines sprinkled in ----
    const char* p5 = "/garbage.tcx";
    {
        std::string junk;
        for (int i = 0; i < 20000; i++) {
            junk.push_back((char)(i % 251));   // includes 0x00, '<', 0xFF...
            if (i % 97 == 96) junk.push_back('\n');
        }
        check(writeAll(sd, p5, junk), "write garbage.tcx");
    }
    {
        TcxLogParser p;
        check(p.open(&sd, p5), "garbage: parser opens");
        check(driveToDone(p), "garbage: parse terminates");
    }

    // ---- Case 6: empty file ----
    const char* p6 = "/empty.tcx";
    check(writeAll(sd, p6, ""), "write empty.tcx");
    {
        TcxLogParser p;
        check(p.open(&sd, p6), "empty: parser opens");
        check(driveToDone(p), "empty: parse terminates");
    }

    // ---- Case 7: CRLF line endings ----
    const char* p7 = "/crlf.tcx";
    {
        std::string doc = std::string(kHead) + trackpoint(10) + kTail;
        std::string crlf;
        crlf.reserve(doc.size() * 2);
        for (char c : doc) { if (c == '\n') crlf += "\r\n"; else crlf += c; }
        check(writeAll(sd, p7, crlf), "write crlf.tcx");
    }
    {
        TcxLogParser p;
        check(p.open(&sd, p7), "crlf: parser opens");
        check(driveToDone(p), "crlf: parse terminates");
        check(p.stats().durationSec == 3600, "crlf: duration == 3600");
    }

    // ---- Case 8: the LogSummaryScreen Del sequence: close, then remove ----
    // The parser destructor now closes its handle (SdFat's own File32
    // destructor does NOT -- DESTRUCTOR_CLOSES_FILE=0), so remove() must
    // succeed afterwards.
    {
        TcxLogParser p;
        check(p.open(&sd, p1), "delete flow: reopen ride1");
        for (int i = 0; i < 32 && p.step(); i++) { /* a few frames' worth */ }
    }   // p's destructor closes the file here
    check(sd.remove(p1), "delete flow: close-then-remove succeeds");
    check(!sd.exists(p1), "delete flow: file gone after remove");

    printf("\n%s (%d failure%s)\n", failures ? "FAILURES" : "ALL PASSED",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}

