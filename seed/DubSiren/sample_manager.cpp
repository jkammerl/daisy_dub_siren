#include "sample_manager.h"

#include "sdcard.h"

int SampleManager::Init(daisy::DaisySeed* seed) {
  sample_player_ = std::shared_ptr<SamplePlayerList>(new SamplePlayerList);
  return 0;
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

int SampleManager::TriggerSample(const SampleInfo& sample_info,
                                 bool retrigger) {
  std::shared_ptr<SamplePlayerList> sample_player_new(new SamplePlayerList);
  auto sample_player_cached = sample_player_;
  *sample_player_new = *sample_player_cached;
  if (retrigger) {
    // Remove currently playing samples.
    auto itr = sample_player_new->begin();
    while (itr != sample_player_new->end()) {
      if ((*itr)->GetSampleInfo().sdfile == sample_info.sdfile) {
        itr = sample_player_new->erase(itr);
      } else {
        ++itr;
      }
    }
  }

  // WavFile* wav_file = nullptr;
  // auto itr = std::find_if(
  //     wav_files_.begin(), wav_files_.end(),
  //     [&sd_file](const WavFile& wf) { return wf.GetSdFile() == sd_file; });
  // if (itr == wav_files_.end()) {
  //   WavFile new_wav_file;
  //   if (new_wav_file.Init(sd_file) == 0 && new_wav_file.IsInitialized()) {
  //     wav_files_.push_back(std::move(new_wav_file));
  //     wav_file = &wav_files_.back();
  //   }
  // } else {
  //   wav_file = &(*itr);
  // }

  std::shared_ptr<SamplePlayer> sample_player(new SamplePlayer(&sample_info));
  sample_player_new->push_back(sample_player);

  while (sample_player_new->size() >= kMaxSimultaneousSample) {
    sample_player_new->pop_front();
  }
  sample_player_ = sample_player_new;
  return 0;
}
