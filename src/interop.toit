import monitor

api ::= InteropApi

class InteropChannel implements SystemMessageHandler_:
  api/InteropApi
  channel_id/int
  channel_base_/int
  init_latch_/monitor.Latch? := monitor.Latch
  handlers_/Map := {:}

  constructor .channel_id .api:
    channel_base_ = INTEROP_MESSAGE_BASE_ + channel_id * INTEROP_MESSAGES_PER_CHANNEL_
    set_system_message_handler_ channel_base_ this
    process_send_ api.external_process_id channel_base_ #[]
    e := catch --unwind=(: it != DEADLINE_EXCEEDED_ERROR):
      with_timeout --ms=500:
        init_latch_.get
        init_latch_ = null
    if e: throw "Native counterpart to channel $channel_id was not detected"

  add_handler message_type/int lambda/Lambda:
    handlers_[message_type] = lambda
    type := type_from_channel_message_type_ message_type
    set_system_message_handler_ type this

  send_message message_type/int data/ByteArray:
    process_send_ api.external_process_id
        type_from_channel_message_type_ message_type
        data

  on_message type/int gid/int pid/int message/any -> none:
    if type == channel_base_:
      init_latch_.set null
    else:
      message_type := channel_message_type_from_type_ type
      handler_ := handlers_.get message_type
      if handler_:
        handler_.call message

  channel_message_type_from_type_ type/int -> int:
    return type - channel_base_ - 1

  type_from_channel_message_type_ message_type/int -> int:
    return message_type + channel_base_ + 1


class InteropApi:
  external_process_id/int

  constructor .external_process_id=0:

  channel id/int:
    return InteropChannel id this


INTEROP_MESSAGE_BASE_         ::= 100000
INTEROP_MESSAGES_PER_CHANNEL_ ::= 1000