#include "classes/wypozyczalnia.h"
#include <locale.h>

using namespace std;

int main() {
    setlocale(LC_CTYPE, "Polish");

    DatabaseConnection dbConnection; // Utworzenie połączenia z bazą danych
    ClientManagement clientManagement(dbConnection);
    GameManagement gameManagement(dbConnection);
    RentalManagement rentalManagement(dbConnection);
    Reporting reporting(dbConnection);

    Menu menu(clientManagement, gameManagement, rentalManagement, reporting);
    menu.displayMenu();
    return 0;
}