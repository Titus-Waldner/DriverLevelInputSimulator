#include <DriverLevelInputSimulator/MouseReport.hpp>

#include <cstdlib>
#include <iostream>

int main()
{
    const DriverLevelInputSimulator::MouseReport report
    {
        0,
        0,
        0,
        0,
        0
    };

    std::cout << "DriverLevelInputSimulator controller\n";
    std::cout << "Mouse report size: "
              << sizeof(report)
              << " bytes\n";
    std::cout << "Driver connection: not implemented\n";

    return EXIT_SUCCESS;
}