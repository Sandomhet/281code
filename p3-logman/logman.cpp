// Project Identifier: 01BD41C3BF016AD7E8B6F837DF18926EC3261350
// EECS 281: Project 3 - Log Manager

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <getopt.h>
using namespace std;


struct LogEntry {
    int64_t timeValue;
    uint32_t entryID;
    string timestamp;
    string category;
    string lowerCategory;
    string message;
};


// The most recent search: either a contiguous range of the sorted master log
// (timestamp searches), a category bucket owned by the category index, or a
// keyword result owned by this struct. All hold indices into the sorted log.
struct SearchResult {
    enum class Kind { None, Range, External, Owned };

    Kind kind = Kind::None;
    uint32_t rangeBegin = 0;
    uint32_t rangeEnd = 0;
    const vector<uint32_t> *external = nullptr;
    vector<uint32_t> owned;

    size_t size() const {
        switch (kind) {
        case Kind::Range:     return rangeEnd - rangeBegin;
        case Kind::External:  return external->size();
        case Kind::Owned:     return owned.size();
        case Kind::None:      break;
        }
        return 0;
    }

    template <typename Func>
    void forEach(Func func) const {
        switch (kind) {
        case Kind::Range:     for (uint32_t i = rangeBegin; i < rangeEnd; ++i) func(i);  break;
        case Kind::External:  for (uint32_t i : *external) func(i);                     break;
        case Kind::Owned:     for (uint32_t i : owned) func(i);                         break;
        case Kind::None:                                                                 break;
        }
    }
};


class LogManager {
public:
    void readLog(const string &filename);
    void run();

private:
    vector<LogEntry> entries;       // sorted by timestamp, category, entryID
    vector<uint32_t> idToSorted;    // entryID -> index in entries
    unordered_map<string, vector<uint32_t>> categoryIndex;
    unordered_map<string, vector<uint32_t>> keywordIndex;
    vector<uint32_t> excerpt;       // indices into entries
    SearchResult lastSearch;

    void buildIndices();

    void setTimeRange(int64_t low, int64_t high);
    void timestampSearch();
    void matchSearch();
    void categorySearch();
    void keywordSearch();
    void appendEntry();
    void appendSearchResults();
    void deleteEntry();
    void moveToBeginning();
    void moveToEnd();
    void sortExcerpt();
    void clearExcerpt();
    void printSearchResults() const;
    void printExcerpt() const;

    void printEntry(uint32_t sortedIdx) const;
    void printExcerptLine(size_t position) const;
    void printExcerptSummary() const;
    bool readExcerptPosition(size_t &position, char command);
};


static int64_t parseTimestamp(const string &ts) {
    int64_t value = 0;
    for (char c : ts) {
        if (c != ':')
            value = value * 10 + (c - '0');
    }
    return value;
}


static char toLower(char c) {
    return static_cast<char>(tolower(static_cast<unsigned char>(c)));
}


static void toLower(string &s) {
    for (char &c : s)
        c = toLower(c);
}


// Calls func on each lowercase alphanumeric word in text.
template <typename Func>
static void forEachWord(const string &text, Func func) {
    string word;
    for (char c : text) {
        if (isalnum(static_cast<unsigned char>(c))) {
            word += toLower(c);
        } else if (!word.empty()) {
            func(word);
            word.clear();
        }
    }
    if (!word.empty())
        func(word);
}


// Reads the remainder of the current command line, without leading spaces.
static string readArgument() {
    string arg;
    getline(cin, arg);
    size_t start = arg.find_first_not_of(' ');
    return start == string::npos ? string() : arg.substr(start);
}


void LogManager::readLog(const string &filename) {
    ifstream fin(filename);
    if (fin.fail()) {
        cerr << "Could not open master log file\n";
        exit(1);
    }

    string line;
    while (getline(fin, line)) {
        size_t bar1 = line.find('|');
        if (bar1 == string::npos)
            continue;
        size_t bar2 = line.find('|', bar1 + 1);

        LogEntry entry;
        entry.entryID = static_cast<uint32_t>(entries.size());
        entry.timestamp = line.substr(0, bar1);
        entry.category = line.substr(bar1 + 1, bar2 - bar1 - 1);
        entry.message = line.substr(bar2 + 1);
        entry.timeValue = parseTimestamp(entry.timestamp);
        entry.lowerCategory = entry.category;
        toLower(entry.lowerCategory);
        entries.push_back(std::move(entry));
    }

    cout << entries.size() << " entries read\n";
    buildIndices();
}


void LogManager::buildIndices() {
    sort(entries.begin(), entries.end(), [](const LogEntry &a, const LogEntry &b) {
        if (a.timeValue != b.timeValue)
            return a.timeValue < b.timeValue;
        if (a.lowerCategory != b.lowerCategory)
            return a.lowerCategory < b.lowerCategory;
        return a.entryID < b.entryID;
    });

    idToSorted.resize(entries.size());
    for (uint32_t i = 0; i < entries.size(); ++i) {
        const LogEntry &entry = entries[i];
        idToSorted[entry.entryID] = i;
        categoryIndex[entry.lowerCategory].push_back(i);

        // Entries are visited in sorted order, so each list stays sorted and
        // only needs a check against its last element to avoid duplicates.
        auto addWord = [this, i](const string &word) {
            vector<uint32_t> &list = keywordIndex[word];
            if (list.empty() || list.back() != i)
                list.push_back(i);
        };
        forEachWord(entry.category, addWord);
        forEachWord(entry.message, addWord);
    }
}


void LogManager::run() {
    char command = '\0';
    do {
        cout << "% ";
        cin >> command;
        if (cin.fail()) {
            cerr << "cin entered fail state: exiting" << endl;
            exit(1);
        }

        string junk;
        switch (command) {
        case 't': timestampSearch();      break;
        case 'm': matchSearch();          break;
        case 'c': categorySearch();       break;
        case 'k': keywordSearch();        break;
        case 'a': appendEntry();          break;
        case 'r': appendSearchResults();  break;
        case 'd': deleteEntry();          break;
        case 'b': moveToBeginning();      break;
        case 'e': moveToEnd();            break;
        case 's': sortExcerpt();          break;
        case 'l': clearExcerpt();         break;
        case 'g': printSearchResults();   break;
        case 'p': printExcerpt();         break;
        case 'q':                         break;
        case '#': getline(cin, junk);     break;
        default:  cerr << "Unknown command: " << command << '\n';  getline(cin, junk);  break;
        }
    } while (command != 'q');
}


// Records every entry with low <= timestamp <= high as the latest search.
void LogManager::setTimeRange(int64_t low, int64_t high) {
    auto first = lower_bound(entries.begin(), entries.end(), low,
                             [](const LogEntry &e, int64_t t) { return e.timeValue < t; });
    auto last = upper_bound(first, entries.end(), high,
                            [](int64_t t, const LogEntry &e) { return t < e.timeValue; });

    lastSearch.kind = SearchResult::Kind::Range;
    lastSearch.rangeBegin = static_cast<uint32_t>(first - entries.begin());
    lastSearch.rangeEnd = static_cast<uint32_t>(last - entries.begin());
}


void LogManager::timestampSearch() {
    string arg = readArgument();
    size_t bar = arg.find('|');
    if (bar != 14 || arg.size() - bar - 1 != 14) {
        cerr << "Invalid timestamps for t command: " << arg << '\n';
        return;
    }

    setTimeRange(parseTimestamp(arg.substr(0, bar)), parseTimestamp(arg.substr(bar + 1)));
    cout << "Timestamps search: " << lastSearch.size() << " entries found\n";
}


void LogManager::matchSearch() {
    string arg = readArgument();
    int64_t target = parseTimestamp(arg);
    setTimeRange(target, target);
    cout << "Timestamp search: " << lastSearch.size() << " entries found\n";
}


void LogManager::categorySearch() {
    string category = readArgument();
    toLower(category);

    static const vector<uint32_t> empty;
    auto it = categoryIndex.find(category);
    lastSearch.kind = SearchResult::Kind::External;
    lastSearch.external = it == categoryIndex.end() ? &empty : &it->second;
    cout << "Category search: " << lastSearch.size() << " entries found\n";
}


void LogManager::keywordSearch() {
    string arg = readArgument();

    vector<const vector<uint32_t> *> lists;
    bool missing = false;
    forEachWord(arg, [&](const string &word) {
        auto it = keywordIndex.find(word);
        if (it == keywordIndex.end())
            missing = true;
        else
            lists.push_back(&it->second);
    });

    lastSearch.kind = SearchResult::Kind::Owned;
    lastSearch.owned.clear();
    if (!missing && !lists.empty()) {
        // Smallest list first keeps every intersection bounded by its size.
        sort(lists.begin(), lists.end(), [](const vector<uint32_t> *a, const vector<uint32_t> *b) {
            return a->size() != b->size() ? a->size() < b->size() : a < b;
        });
        lists.erase(unique(lists.begin(), lists.end()), lists.end());

        lastSearch.owned = *lists.front();
        vector<uint32_t> scratch;
        for (size_t i = 1; i < lists.size() && !lastSearch.owned.empty(); ++i) {
            scratch.clear();
            set_intersection(lastSearch.owned.begin(), lastSearch.owned.end(),
                             lists[i]->begin(), lists[i]->end(), back_inserter(scratch));
            lastSearch.owned.swap(scratch);
        }
    }
    cout << "Keyword search: " << lastSearch.size() << " entries found\n";
}


void LogManager::appendEntry() {
    long long entryID = 0;
    cin >> entryID;
    if (entryID < 0 || entryID >= static_cast<long long>(entries.size())) {
        cerr << "Invalid entryID for a command: " << entryID << '\n';
        return;
    }

    excerpt.push_back(idToSorted[static_cast<size_t>(entryID)]);
    cout << "log entry " << entryID << " appended\n";
}


void LogManager::appendSearchResults() {
    if (lastSearch.kind == SearchResult::Kind::None) {
        cerr << "No previous search for r command\n";
        return;
    }

    excerpt.reserve(excerpt.size() + lastSearch.size());
    lastSearch.forEach([this](uint32_t i) { excerpt.push_back(i); });
    cout << lastSearch.size() << " log entries appended\n";
}


bool LogManager::readExcerptPosition(size_t &position, char command) {
    long long value = 0;
    cin >> value;
    if (value < 0 || value >= static_cast<long long>(excerpt.size())) {
        cerr << "Invalid excerpt list position for " << command << " command: " << value << '\n';
        return false;
    }
    position = static_cast<size_t>(value);
    return true;
}


void LogManager::deleteEntry() {
    size_t position = 0;
    if (!readExcerptPosition(position, 'd'))
        return;

    excerpt.erase(excerpt.begin() + static_cast<ptrdiff_t>(position));
    cout << "Deleted excerpt list entry " << position << '\n';
}


void LogManager::moveToBeginning() {
    size_t position = 0;
    if (!readExcerptPosition(position, 'b'))
        return;

    auto it = excerpt.begin() + static_cast<ptrdiff_t>(position);
    rotate(excerpt.begin(), it, it + 1);
    cout << "Moved excerpt list entry " << position << '\n';
}


void LogManager::moveToEnd() {
    size_t position = 0;
    if (!readExcerptPosition(position, 'e'))
        return;

    auto it = excerpt.begin() + static_cast<ptrdiff_t>(position);
    rotate(it, it + 1, excerpt.end());
    cout << "Moved excerpt list entry " << position << '\n';
}


void LogManager::sortExcerpt() {
    cout << "excerpt list sorted\n";
    if (excerpt.empty()) {
        cout << "(previously empty)\n";
        return;
    }

    cout << "previous ordering:\n";
    printExcerptSummary();
    // Indices into the sorted master log already compare in the required order.
    sort(excerpt.begin(), excerpt.end());
    cout << "new ordering:\n";
    printExcerptSummary();
}


void LogManager::clearExcerpt() {
    cout << "excerpt list cleared\n";
    if (excerpt.empty()) {
        cout << "(previously empty)\n";
        return;
    }

    cout << "previous contents:\n";
    printExcerptSummary();
    excerpt.clear();
}


void LogManager::printSearchResults() const {
    if (lastSearch.kind == SearchResult::Kind::None) {
        cerr << "No previous search for g command\n";
        return;
    }

    lastSearch.forEach([this](uint32_t i) { printEntry(i); });
}


void LogManager::printExcerpt() const {
    for (size_t position = 0; position < excerpt.size(); ++position)
        printExcerptLine(position);
}


void LogManager::printEntry(uint32_t sortedIdx) const {
    const LogEntry &entry = entries[sortedIdx];
    cout << entry.entryID << '|' << entry.timestamp << '|' << entry.category << '|' << entry.message
         << '\n';
}


void LogManager::printExcerptLine(size_t position) const {
    cout << position << '|';
    printEntry(excerpt[position]);
}


void LogManager::printExcerptSummary() const {
    printExcerptLine(0);
    cout << "...\n";
    printExcerptLine(excerpt.size() - 1);
}


void printHelp(char *command) {
    cout << "Usage: " << command << " -f LOGFILE < commands\n"
         << "Reads a master log file, then runs search and excerpt list commands from stdin.\n"
         << "  -h, --help             Print this message and exit\n"
         << "  -f, --file LOGFILE     Read log entries from LOGFILE\n"
         << flush;
}


string getLogFile(int argc, char **argv) {
    opterr = static_cast<int>(false);
    int choice = 0;
    int index = 0;
    string filename;

    option longOptions[] = {
        {"help",       no_argument, nullptr, 'h'},
        {"file", required_argument, nullptr, 'f'},
        {nullptr,                0, nullptr, '\0'},
    };

    while ((choice = getopt_long(argc, argv, "hf:", longOptions, &index)) != -1) {
        switch (choice) {
        case 'h': printHelp(*argv);  exit(0);
        case 'f': filename = optarg;  break;
        default:  cerr << "Error: invalid option\n";  exit(1);
        }
    }

    if (filename.empty()) {
        cerr << "Error: no log file specified (use -f LOGFILE)\n";
        exit(1);
    }
    return filename;
}


int main(int argc, char *argv[]) {
    ios_base::sync_with_stdio(false);

    string filename = getLogFile(argc, argv);
    LogManager manager;
    manager.readLog(filename);
    manager.run();
    return 0;
}
