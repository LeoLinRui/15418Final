#include <Fade_2D.h>
using namespace GEOM_FADE2D;

int main() {
    Fade_2D obj1;
    Fade_2D obj2 = obj1;  // Test copy constructor
    //Fade_2D obj3 = std::move(obj1);  // Test move constructor
    return 0;
}