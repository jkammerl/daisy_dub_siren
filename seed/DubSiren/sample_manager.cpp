#include "sample_manager.h"

#include <array>

#include "sdcard.h"

int SampleManager::Init(daisy::DaisySeed* seed) {
  InitSdCard();
  ReadDir(files_, {".wav", ".WAV", ".Wav"}, true);
  std::sort(files_.begin(), files_.end());
  seed->PrintLine("Num samples found %d", files_.size());

  sample_player_ = std::shared_ptr<SamplePlayerList>(new SamplePlayerList);

  wav_files_.reserve(files_.size());
  play_list_.resize(6 * 4, nullptr);
  for (size_t i = 0; i < files_.size(); ++i) {
    int index = GetIndex(files_[i]);
    if (index < 0) {
      continue;
    }
    WavFile new_streamer;
    int err = new_streamer.Init(files_[i]);
    if (err == 0 && new_streamer.IsInitialized()) {
      wav_files_.push_back(std::move(new_streamer));
      play_list_[index] = &wav_files_.back();
      seed->PrintLine(files_[i].c_str());
    } else {
      seed->PrintLine("Streamer not initialized %d", files_.size());
    }
  }

  return 0;
}

int SampleManager::GetIndex(const std::string& filename) {
  if (filename.size() < 4) {
    return -1;
  }
  if (filename[0] != '/') {
    return -1;
  }
  if (filename[1] < '1' && filename[1] > '6') {
    return -1;
  }
  int bank = filename[1] - '1';
  if (filename[2] != '/') {
    return -1;
  }
  if (filename[3] < '1' && filename[3] > '4') {
    return -1;
  }
  int sample = filename[3] - '1';
  int idx = bank * 4 + sample;
  static std::array<bool, 6 * 4> seen = {};
  if (seen[idx]) {
    return -1;
  }
  seen[idx] = true;

  return idx;
}

float SampleManager::GetSample() {
  auto sample_player_new = sample_player_;
  float sample_player_sum = 0.0f;
  for (auto sample_player : *sample_player_new) {
    sample_player_sum += sample_player->GetSample() / 32768.0f;
  }
  return sample_player_sum;
}

int SampleManager::SdCardLoading() {
  int err = 0;
  bool has_finished_players = false;
  for (auto& sample_player : *sample_player_) {
    err = sample_player->Prepare();
    if (err != 0) {
      return err;
    }
    has_finished_players |= !sample_player->IsPlaying();
  }

  if (has_finished_players) {
    std::shared_ptr<SamplePlayerList> sample_player_new(new SamplePlayerList);
    auto sample_player_cached = sample_player_;
    *sample_player_new = *sample_player_cached;
    // Remove finished samples.
    auto itr = sample_player_new->begin();
    while (itr != sample_player_new->end()) {
      const bool is_playing = (*itr)->IsPlaying();
      if (!is_playing) {
        itr = sample_player_new->erase(itr);
      } else {
        ++itr;
      }
    }
    sample_player_ = sample_player_new;
  }
  return 0;
}

int SampleManager::TriggerSample(int sample_idx, bool retrigger) {
  if (sample_idx < 0 || sample_idx >= static_cast<int>(play_list_.size())) {
    return -1;
  }
  std::shared_ptr<SamplePlayerList> sample_player_new(new SamplePlayerList);
  auto sample_player_cached = sample_player_;
  *sample_player_new = *sample_player_cached;
  if (retrigger) {
    // Remove currently playing samples.
    auto itr = sample_player_new->begin();
    while (itr != sample_player_new->end()) {
      if ((*itr)->GetSampleIdx() == sample_idx) {
        itr = sample_player_new->erase(itr);
      } else {
        ++itr;
      }
    }
  }
  WavFile* wav = play_list_[sample_idx];
  if (wav != nullptr) {
    sample_player_new->push_back(wav->GetSamplePlayer(sample_idx));
  }

  while (sample_player_new->size() >= kMaxSimultaneousSample) {
    sample_player_new->pop_front();
  }
  sample_player_ = sample_player_new;
  return 0;
}
