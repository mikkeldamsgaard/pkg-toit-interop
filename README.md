# pkg-toit-interop
A take on how to integrate the Toit VM into a custom idf application 
and setting up messaging.

### Basic usage
On the idf side, setup a CMakeLists.txt a la this
    
    cmake_minimum_required(VERSION 3.5)
    list(APPEND EXTRA_COMPONENT_DIRS  "$TOIT_REPO_PATH/toolchains/idf/components" "pkg-toit-interop")
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(example)
    toit_postprocess()

Then clone the pkg-toit-interop repo:
    
    git clone https://github/mikkeldamsgaard/pkg-toit-interop

In you `main.c` add the following code to boot the VM

``` c++
#include "esp_system.h"
#include <toit_interop.h>

void app_main(void) {
    toit::Interop interop(0);
    interop.run_vm();
}
```

### Adding a channel
The package uses channels to allow communication between toit 
and the idf code. A channel has an id, and the number of channels used
is a parameter to the constructor of the Interop class.

To add a channel to the idf side, then just define it, as a simple echo
example see this code

``` c++
class TestChannel: public toit::InteropChannel {
public:
  TestChannel(toit::Interop* interop, int channel_id): InteropChannel(interop, channel_id) {}
  bool receive(int type, const void* data, int length) override {
    send(type, const_cast<void *>(data), length);
    return false;
  }
};

extern "C" {
void app_main(void) {
  toit::Interop interop(1);
  TestChannel test_channel(&interop, 0);

  interop.run_vm();
}
}
```

On the toit side, you will have to add a package dependency to the toit-interop

    toit.pkg install github.com/mikkeldamsgaard/pkg-toit-interop
 
And then on you toit code you will need to do
``` toit
import interop

MSG_TYPE_PING ::= 0
main:
  channel/interop.InteropChannel := interop.api.channel 0

  channel.add_handler MSG_TYPE_PING :: print "Ping reply from idf: $it.to_string_non_throwing"

  print "Send hello world ping msg to idf"
  channel.send_message MSG_TYPE_PING "hello world".to_byte_array
```

In the console of the device, the above should be printing

    Send hello world ping msg to idf
    Ping reply from idf: hello world    