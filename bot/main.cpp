#include <iostream>

using namespace std;

void run_bot();

int main()
{
    try {
        run_bot();
    } catch (const exception& e) {
        cerr << "Exception caught: " << e.what() << endl;
    }
    return 0;
}
