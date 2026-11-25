#include "model.h"
#include "view.h"

int main(void)
{
    Manager manager;

    initManager(&manager);

    runPhonebook(&manager);

    destroyManager(&manager);

    return 0;
}