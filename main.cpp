#include <stdio.h>
#include <dlfcn.h>

int main()
{
  printf("Test\n");

  void* handle = dlopen("libVerdande.so",RTLD_LAZY);

  if (handle == nullptr){
    printf("Feil: %s\n", dlerror());
    return 1;
  }
  
  void (*hello)(void) = reinterpret_cast<void (*)()>(dlsym(handle, "hello"));

  hello();
  dlclose(handle);
}
