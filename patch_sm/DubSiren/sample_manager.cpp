#include "sample_manager.h"

#include "sdcard.h"

int SampleManager::Init() {
  InitSdCard();
  ReadDir(files_, {".wav", ".WAV", ".Wav"}, true);

  sample_player_ = std::shared_ptr<SamplePlayerList>(new SamplePlayerList);

  wav_files_.reserve(files_.size());
  for (size_t i = 0; i < files_.size(); ++i) {
    const auto& filename = files_[i];
    WavFile new_streamer;
    int err = new_streamer.Init(filename.c_str());
    if (new_streamer.IsInitialized()) {
      wav_files_.push_back(std::move(new_streamer));
    }
    if (err != 0) {
      return err;
    }
  }

  return 0;
}

float SampleManager::GetSample() {
  auto sample_player_copy = sample_player_;
  float sample_player_sum = 0.0f;
  for (auto sample_player : *sample_player_copy) {
    sample_player_sum += s162f(sample_player->GetSample());
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
    std::shared_ptr<SamplePlayerList> sample_player_copy(new SamplePlayerList);
    *sample_player_copy = *sample_player_;
    // Remove finished samples.
    auto itr = sample_player_copy->begin();
    while (itr != sample_player_copy->end()) {
      const bool is_playing = (*itr)->IsPlaying();
      if (!is_playing) {
        itr = sample_player_copy->erase(itr);
      } else {
        ++itr;
      }
    }
    sample_player_ = sample_player_copy;
  }
  return 0;
}

int SampleManager::TriggerSample(int sample_idx, bool retrigger) {
  if (sample_idx < 0 || sample_idx >= static_cast<int>(wav_files_.size())) {
    return -1;
  }
  std::shared_ptr<SamplePlayerList> sample_player_copy(new SamplePlayerList);
  *sample_player_copy = *sample_player_;
  if (retrigger) {
    // Remove currently playing samples.
    auto itr = sample_player_copy->begin();
    while (itr != sample_player_copy->end()) {
      if ((*itr)->GetSampleIdx() == sample_idx) {
        itr = sample_player_copy->erase(itr);
      } else {
        ++itr;
      }
    }
  }
  sample_player_copy->push_back(
      wav_files_[sample_idx].GetSamplePlayer(sample_idx));

  while (sample_player_copy->size() >= kMaxSimultaneousSample) {
    sample_player_copy->pop_front();
  }
  sample_player_ = sample_player_copy;
  return 0;
}
