#include <iostream>
#include <Windows.h>

int main()
{
    auto hDLL = LoadLibraryA("..\\Assembly64.TestAsi\\Assembly64.TestAsi.asi");
    if (hDLL != NULL)
    {
        auto procedure = (void(*)())GetProcAddress(hDLL, "InitializeASI");
        if (procedure != NULL)
            procedure();

        MessageBoxA(0, "1234", "5678", 0);
        MessageBoxA(0, "Hello World!\n", "Hello World!\n", 0);
        while (true)
        {
            Sleep(500);
            std::cout << "Hello World!\n";
        }
    }
    return 0;
}