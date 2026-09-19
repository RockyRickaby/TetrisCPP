#pragma once

#include <SDL3/SDL_log.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream> 
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <format>
#include <cstdint>
#include <vector>

namespace Tetris::Score {
    // size multiple of 16 bytes makes it a bit easier to read on hex editors
    struct ScoreEntry {
        static inline constexpr int player_name_max_length = 7;
        std::int64_t score;
        // 9 chars, last is for null
        std::array<char, player_name_max_length + 1> player_name;
    };

    using LSCallback = std::function<void(ScoreEntry&)>;
    // using LSCallback = decltype([](std::vector<ScoreEntry>&) -> void{});

    template<typename T, typename It>
    concept LeaderboardStorage = requires(T storage, LSCallback callback, It new_scores_begin, It new_scores_end) {
        { storage.load(callback) } -> std::same_as<bool>;
        { storage.save(new_scores_begin, new_scores_end) } -> std::same_as<bool>;
        { T::type() } -> std::same_as<std::string>;
    };

    class OnDiskLeaderboard {
    public:
        OnDiskLeaderboard(const std::filesystem::path& path)  :
            m_data{},
            m_path{path}
        {}

        static std::string type() {
            return "OnDiskLeaderboard";
        }

        template<typename Callback>
        bool load(Callback fn) {
            namespace fs = std::filesystem;
            constexpr int entry_size = sizeof(ScoreEntry);
            if (m_data.size() > 0) {
                std::ranges::for_each(m_data, fn);
                return true;
            }

            if (!fs::exists(m_path)) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, std::format("file '{}' does not exist", m_path.string()).c_str());
                m_data.clear();
                return false;
            }
            
            std::filebuf file;
            file.open(m_path, std::ios::in | std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error(std::format("could not open file: {}",  m_path.string()));
            }

            std::streampos size = file.pubseekoff(0, std::ios::end, std::ios::in);
            file.pubseekoff(0, std::ios::beg, std::ios::in);

            std::vector<char> entries;
            entries.resize(size);
            file.sgetn(entries.data(), size);
            file.close();
            for (int i = 0; i < size; i += entry_size) {
                ScoreEntry dto_entry;
                std::memcpy(&dto_entry, entries.data() + i, entry_size);
                m_data.emplace_back(std::move(dto_entry));
                fn(m_data.back());
            }
            return true;
        }

        template<typename It>
        bool save(It scores_begin, It scores_end) {
            std::filebuf file;
            file.open(m_path, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!file.is_open()) {
                throw std::runtime_error(std::format("could not find path: {}", m_path.string()));
            }
            constexpr int entry_size = sizeof(ScoreEntry);
            auto size = std::distance(scores_begin, scores_end);
            ptrdiff_t off = 0;
            std::vector<char> entries;
            entries.resize(entry_size * size);

            m_data.clear();
            m_data.reserve(size);
            for (; scores_begin != scores_end; ++scores_begin) {
                auto& [val, name] = *scores_begin;
                ScoreEntry score{val, {}};
                score.player_name.fill(0);

                const auto end = name.size() > ScoreEntry::player_name_max_length ? name.cbegin() + ScoreEntry::player_name_max_length : name.cend();
                std::copy(name.cbegin(), end, score.player_name.begin());
                m_data.push_back(score);

                std::memcpy(entries.data() + off, &score, entry_size);
                off += entry_size;
            }
            file.sputn(entries.data(), entries.size());
            return true;
        }
    private:
        std::vector<ScoreEntry> m_data;
        std::filesystem::path m_path;

    public:
    };

    class InMemoryLeaderboard {
    public:
        InMemoryLeaderboard() :
            m_scores{
                {120000, "tetris"},
                {100000, "mauro"},
                {80000, "da goat"},
                {70000, "rohan"},
                {60000, "makr"},
                {50000, "daniel"},
                {40000, "hwg"},
                {30000, "kif"},
                {20000, "vicky"},
                {10000, "charles"},
            }
        {}

        static std::string type() {
            return "InMemoryLeaderboard";
        }

        template<typename Callback>
        bool load(Callback fn) {
            std::ranges::for_each(m_scores, fn);
            return true;
        }

        template<typename It>
        bool save(It scores_begin, It scores_end) {
            m_scores.clear();
            for (; scores_begin != scores_end; ++scores_begin) {
                auto& [val, name] = *scores_begin;
                ScoreEntry score{val, {}};
                score.player_name.fill(0);
    
                const auto end = name.size() > ScoreEntry::player_name_max_length ? name.cbegin() + ScoreEntry::player_name_max_length : name.cend();
                std::copy(name.cbegin(), end, score.player_name.begin());

                m_scores.emplace_back(std::move(score));
            }
            return true;
        }
    private:
        std::vector<ScoreEntry> m_scores;
    };





    // terrible. terrible interface. terrible implementation. should be improved
    class Leaderboard {
    private:
        // std::map<std::int64_t, std::string, std::less<std::int64_t>> m_scores;
        // using iterator_type = decltype(m_scores)::iterator;
        std::vector<std::pair<std::int64_t, std::string>> m_scores;
        using iterator_type = decltype(m_scores)::reverse_iterator;
    public:
        size_t size() const {
            return m_scores.size();
        }

        std::int64_t top_score() const {
            if (m_scores.empty()) {
                return -1;
            }
            return std::get<std::int64_t>(m_scores.back());
        }

        bool is_new_highscore(std::int64_t score) const {
            if (score <= 0) {
                return false;
            }
            auto it = std::ranges::lower_bound(m_scores, std::make_pair(score, ""), [](const auto& lhs, const auto& rhs){
                return std::get<0>(lhs) < std::get<0>(rhs);
            });

            if (std::ranges::distance(it, std::ranges::end(m_scores)) < 10) {
                return true;
            }
            return false;
        }

        bool push_score(std::int64_t score, const std::string& player_name) {
            if (score <= 0) {
                return false;
            }
            // m_scores.insert_or_assign(score, player_name.substr(0, ScoreEntry::player_name_max_length));
            const auto t = std::make_pair(score,player_name.substr(0, ScoreEntry::player_name_max_length));
            m_scores.insert(std::ranges::lower_bound(
                m_scores, t, [](const auto& lhs, const auto& rhs){
                    return std::get<0>(lhs) < std::get<0>(rhs);
                }), t
            );
            if (m_scores.size() > 10) {
                m_scores.erase(m_scores.begin());
            }
            return true;
        }

        void clear_scores() { m_scores.clear(); }

        template<LeaderboardStorage<iterator_type> Storage>
        void flush_scores(Storage& s) {
            s.save(m_scores.rbegin(), m_scores.rend());
        }

        template<LeaderboardStorage<iterator_type> Storage>
        void load_scores(Storage& s) {
            clear_scores();
            const auto cb = [this](const ScoreEntry& data){
                // m_scores.emplace(data.score, std::string{data.player_name.data()});
                m_scores.emplace_back(data.score, std::string(data.player_name.data()));
            };
            bool success = s.load(cb);
            // if we fail to load from whatever Storage type that was passed in,
            // load the values from the in-memory storage as a fallback.
            // saving (and even loading) should then work just fine
            if (!success) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, std::format("loading scores from {} failed. Falling back to reading from {} instead.", Storage::type(), InMemoryLeaderboard::type()).c_str());
                Score::InMemoryLeaderboard{}.load(cb);
            }
            std::ranges::sort(m_scores, [](const auto& lhs, const auto& rhs){
                return std::get<0>(lhs) < std::get<0>(rhs);
            });
        }

        iterator_type begin() { return m_scores.rbegin(); }
        iterator_type end() { return m_scores.rend(); }
    };
}