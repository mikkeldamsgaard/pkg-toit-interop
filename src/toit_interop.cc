#include "toit_interop.h"
#include "os.h"
#include "flash_registry.h"
#include "rtc_memory_esp32.h"
#include "embedded_data.h"

namespace toit {

const int INTEROP_MESSAGE_BASE_ = 100000;
const int INTEROP_MESSAGES_PER_CHANNEL_ = 1000;

Interop::Interop(uint8 num_channels): ExternalSystemMessageHandler(&vm_), num_channels_(num_channels) {
  vm_.load_platform_event_sources();
  boot_group_id_ = vm_.scheduler()->next_group_id();
  channels_ = _new InteropChannel*[num_channels];
}

Interop::~Interop() {
  delete channels_;
}

bool Interop::initialized_ = false;

void Interop::set_up() {
  if (initialized_) return;
  initialized_ = true;
  RtcMemory::set_up();
  FlashRegistry::set_up();
  OS::set_up();
  ObjectMemory::set_up();
}

Scheduler::ExitState Interop::run_vm(uint8 priority) {
  start(priority);
  const EmbeddedDataExtension* extension = EmbeddedData::extension();
  EmbeddedImage boot = extension->image(0);
  return vm_.scheduler()->run_boot_program(const_cast<Program*>(boot.program), boot_group_id_);
}

void Interop::on_message(int sender, int system_message_type, void* data, int length) {
  // Unpack channel and type from system_message_type
  int channel = (system_message_type - INTEROP_MESSAGE_BASE_) / INTEROP_MESSAGES_PER_CHANNEL_;
  int type = system_message_type - (INTEROP_MESSAGE_BASE_ + channel * INTEROP_MESSAGES_PER_CHANNEL_ + 1);

  if (channel < 0 || channel >= num_channels_ || !channels_[channel]) {
    fail("Invalid message received in interop, system message type was %d, decoded to channel %d and type %d",
         system_message_type, channel, type);
  }
  InteropChannel* interop_channel = channels_[channel];
  if (type == 0) {
    interop_channel->set_current_process_id(sender);
    auto dummy = static_cast<uint8*>(malloc(1));
    ExternalSystemMessageHandler::send(sender, system_message_type, dummy, 1);
  } else {
    channels_[channel]->receive(type, data, length);
  }
}

void Interop::register_channel(InteropChannel* channel) {
  if (channel->channel_id() >= num_channels_) {
    fail("Supplied channel id %d is outside allowed range 0-%d. Did you forget to update construction parameter?",
         channel->channel_id(),
         num_channels_-1);
  }
  channels_[channel->channel_id()] = channel;
}

bool Interop::send(InteropChannel* channel, int type, void* buf, int length) {
  int pid = channel->current_process_id();

  if (pid == -1) {
    toit::fail("Sending message before receiver is registered");
  }

  if (type < 1 || type >= INTEROP_MESSAGES_PER_CHANNEL_) {
    toit::fail("type is out of range 1-%d",INTEROP_MESSAGES_PER_CHANNEL_-1);
  }

  int message_system_type = INTEROP_MESSAGE_BASE_ + INTEROP_MESSAGES_PER_CHANNEL_ * channel->channel_id() + 1 + type;
  return ExternalSystemMessageHandler::send(pid, message_system_type, buf, length);
}


}