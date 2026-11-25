
#include "calculator.h"
#include "view.h"
#include "operators.h"

#include <stdio.h>

int main(void) {
    Calculator calculator = {0};

    printf("Loading calculator plugins...\n");

    int loaded = operators_load_directory("./plugins");

    if (loaded == 0) {
        fprintf(stderr, "Error: No calculator operations loaded.\n");
        return 1;
    }

    printf("Successfully loaded %d operations.\n\n", loaded);

    run_calculator(&calculator);

    operators_unload_all();

    return 0;
}
