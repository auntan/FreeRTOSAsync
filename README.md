Header only, zero heap allocation future/promise library built on top of FreeRTOSHelpers

## Usage

```cpp
#include <include/freertosasync.hpp>

int main() {
    FreeRTOSAsync::setImmediate([]() {
        printf("called immediate\n");
        return FreeRTOSAsync::setTimeout(1000, []() {
            printf("called after 1000 ms\n");
            return FreeRTOSAsync::Future();
        });
    }).then([]() {
        printf("called immediate\n");
        return FreeRTOSAsync::setTimeout(1000, []() {
            printf("called after 1000 ms\n");
            return FreeRTOSAsync::Future();
        });
    }).then([]() {
        printf("called immediate\n");
        return FreeRTOSAsync::Future();
    }).then([]() {
        printf("end\n");
        return FreeRTOSAsync::Future();
    });
    
    printf("begin\n");
            
    // FreeRTOS scheduler must be run
    vTaskStartScheduler();
}
```
Output:
```
[00.000] begin
[00.000] called immediate
[01.000] called after 1000 ms
[01.000] called immediate
[02.000] called after 1000 ms
[02.000] called immediate
[02.000] end
```
