#include "model.h"
#include "view.h"

int main(void)
{
    Manager manager;

    initManager(&manager);

    runView(&manager);

    freeTree(manager.root);

    return 0;
}