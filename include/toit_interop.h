#pragma once

#include <top.h>
#include <vm.h>
#include <scheduler.h>

namespace toit {

class InteropChannel;

class Interop : public ExternalSystemMessageHandler {
 public:
  Interop(uint8 num_channels);
  ~Interop();

  // This call blocks until the toit program exists.
  Scheduler::ExitState run_vm(uint8 priority = Process::PRIORITY_NORMAL);

  static void set_up();

 private:
  void on_message(int sender, int type, void* data, int length) override;
  void register_channel(InteropChannel* channel);
  bool send(InteropChannel* channel, int type, void* buf, int length);
  VM vm_;
  int boot_group_id_;
  InteropChannel** channels_;
  int num_channels_;

  static bool initialized_;

  friend class InteropChannel;
};

class InteropChannel {
 public:
  InteropChannel(Interop* interop, int channel_id) : channel_id_(channel_id), interop_(interop) {
    interop_->register_channel(this);
  }

  // Send the byte buffer data to the toit VM.
  //
  // On success this transfers ownership, as the VM needs to be able to
  // manage the memory after the buffer has been delivered.
  //
  // In case of error the ownership of the buffer is returned to the caller.
  bool send(int type, void* buf, int length) {
    return interop_->send(this, type, buf, length);
  }

  int channel_id() const { return channel_id_; }

  virtual bool receive(int type, const void* data, int length) = 0;

 private:
  void set_current_process_id(int process_id) { current_process_id_ = process_id; }
  int current_process_id() const { return current_process_id_; }

  const int channel_id_;
  int current_process_id_ = -1;
  Interop* interop_;

  friend class Interop;
};

}