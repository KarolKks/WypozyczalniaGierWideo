#include <iostream>
#include <mysql.h>
#include <string>
#include <cstdlib>
#include <windows.h>
#include <fstream>
#include <ctime>
#include "wypozyczalnia.h"


using namespace std;

DatabaseConnection::DatabaseConnection(const string& host, const string& user,
    const string& password, const string& database, unsigned int port) {
    connection_ = mysql_init(NULL);
    if (connection_ == NULL) {
        cout << "Błąd inicjalizacji MySQL: " << mysql_error(connection_) << endl;
        exit(EXIT_FAILURE);
    }

    if (mysql_real_connect(connection_, host.c_str(), user.c_str(), password.c_str(),
        database.c_str(), port, NULL, 0) == NULL) {
        cout << "Błąd połączenia z bazą danych: " << mysql_error(connection_) << endl;
        mysql_close(connection_);
        exit(EXIT_FAILURE);
    }

    if (mysql_set_character_set(connection_, "utf8mb4") != 0) {
        cout << "Błąd ustawiania kodowania UTF-8MB4: " << mysql_error(connection_) << endl;
        mysql_close(connection_);
        exit(EXIT_FAILURE);
    }
}

DatabaseConnection::~DatabaseConnection() {
    mysql_close(connection_);
}

MYSQL* DatabaseConnection::getConnection() const {
    return connection_;
}

void Management::logAction(const string& action) {
    ofstream logFile("logi.txt", std::ios::app); // Tryb dopisania
    if (logFile.is_open()) {
        time_t now = time(0); // Pobranie aktualnego czasu
        struct tm localTime;

        // Użycie localtime_s zamiast localtime
        if (localtime_s(&localTime, &now) == 0) { // Sprawdzanie, czy funkcja wykonała się poprawnie
            char dateTime[20];
            strftime(dateTime, sizeof(dateTime), "%Y-%m-%d %H:%M:%S", &localTime);

            logFile << "[" << dateTime << "] Wykonano akcję: " << action << std::endl;
        }
        else {
            logFile << "[Błąd czasu] Wykonano akcję: " << action << std::endl;
        }

        logFile.close();
    }
    else {
        std::cout << "Nie udało się otworzyć pliku logów." << std::endl;
    }
}

void Management::back() {
    string q;

    while (true) {
        cout << "Wpisz 'Q' aby wyjść do menu głównego" << endl;
        cin >> q;
        if (q == "Q" || q == "q") {
            break;
        }
        else {
            cout << "Nieprawidłowy znak." << endl;
        }

    }

}

bool Management::checkIfIdExists(MYSQL* conn, const string& tableName, string id) {
    string query = "SELECT COUNT(*) FROM " + tableName + " WHERE id = " + id + ";";
    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd podczas sprawdzania ID: " << mysql_error(conn) << endl;
        return false; // W przypadku błędu zakładamy, że ID nie istnieje.
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        cout << "Błąd przechowywania wyników: " << mysql_error(conn) << endl;
        return false; // Brak wyników traktujemy jako brak ID.
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    bool exists = (row && atoi(row[0]) > 0); // Jeśli wynik > 0, ID istnieje.
    mysql_free_result(result);

    return exists;
}

bool Management::checkIfIdInRelation(MYSQL* conn, const string& column, const string& id) {
    // Użyjemy poprawnego zapytania SQL
    string query = "SELECT COUNT(*) FROM rentals WHERE " + column + " = '" + id + "';";

    // Wykonanie zapytania
    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd zapytania: " << mysql_error(conn) << endl;
        return false;
    }

    // Pobranie wyniku
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        cout << "Błąd pobierania wyników: " << mysql_error(conn) << endl;
        return false;
    }

    // Pobranie pierwszego wiersza wyniku
    MYSQL_ROW row = mysql_fetch_row(result);
    bool linked = false;
    if (row && row[0]) {
        linked = atoi(row[0]) > 0;
    }

    // Zwolnienie pamięci wyniku
    mysql_free_result(result);
    return linked;
}


// Zarządzanie profilami klientów
ClientManagement::ClientManagement(DatabaseConnection& dbConn) : dbConnection(dbConn) {}

void ClientManagement::add() {
    MYSQL* conn = dbConnection.getConnection();
    string name, lastname, phoneNumber;
    cout << "Przy podawaniu imienia i nazwiska proszę nie podawać polskich znaków.\n";
    cout << "Wprowadź imię: ";
    cin >> name;
    cout << "Wprowadź nazwisko: ";
    cin >> lastname;
    cout << "Wprowadź numer telefonu: ";
    cin >> phoneNumber;

    for (char s : phoneNumber) {
        if (s >= 48 && s <= 57) {

        }
        else {
            cout << "Niepoprawnie podany numer telefonu, ponów operacje." << endl;
            return;
        }
    }

    if (phoneNumber.length() == 9) {

    }
    else {
        cout << "Niepoprawna długość numeru telefonu, ponów operacje" << endl;
        return;
    }


    string query = "INSERT INTO clients(name, lastname, phoneNumber) VALUES('" + name + "','" + lastname + "'," + phoneNumber + ");";

    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd dodawania klienta: " << mysql_error(conn) << endl;
    }
    else {
        cout << "Klient dodany!" << endl;
        logAction("Dodano klienta");
    }
}
void ClientManagement::remove() {
    cout << "-----Obecni klienci-----" << endl;
    ClientManagement::show();
    MYSQL* conn = dbConnection.getConnection();
    string id;
    cout << "Wprowadź ID klienta do usunięcia: ";
    cin >> id;

    if (!checkIfIdExists(conn, "clients", id)) {
        cout << "Nie istnieje klient o padnym id. Spróbuj ponownie." << endl;
        return;
    }

    if (Management::checkIfIdInRelation(conn, "client_id", id)) {
        cout << "Nie można usunąć klienta, ponieważ jest powiązany z wypożyczeniami!" << endl;
        return;
    }

    string query = "DELETE FROM clients WHERE id = " + id + ";";

    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd usuwania klienta: " << mysql_error(conn) << endl;
    }
    else {
        cout << "Klient usunięty!" << endl;
        logAction("Usunięto klienta");
    }
}

void ClientManagement::update() {
    MYSQL* conn = dbConnection.getConnection();

    if (conn == nullptr) {
        cout << "Błąd połączenia z bazą danych!" << endl;
        return;
    }

    string id, new_name, newlastname, newPhoneNumber;

    while (true) {

        cout << "Wpisz '0', żeby wyjść.\n";
        cout << "Wprowadź ID klienta do zaktualizowania lub naciśnij 'F' aby wyświetlić listę: ";
        cin >> id;


        if (id == "0") return;
        if (id == "F" or id == "f")
        {
            cout << endl;
            cout << "-----Aktualni klienci-----" << endl;
            ClientManagement::show();
            continue;
        }

        int clientId = stoi(id);
        if (!checkIfIdExists(conn, "clients", id)) {
            cout << "Nie istnieje klient o padnym id. Spróbuj ponownie." << endl;
            return;
        }
        cout << "\nWprowadź imię lub naciśnij Enter aby pominąć: ";
        cin.ignore();
        getline(cin, new_name);
        if (new_name == "0") return;
        cout << "Wprowadź nazwisko lub naciśnij Enter aby pominąć: ";
        getline(cin, newlastname);
        if (newlastname == "0") return;
        cout << "Wprowadź nowy numer telefonu lub naciśnij Enter aby pominąć: ";
        getline(cin, newPhoneNumber);
        if (newPhoneNumber == "0") return;

        string query = "UPDATE clients SET ";
        bool firstField = true;

        if (!new_name.empty()) {
            query += "name = '" + new_name + "'";
            firstField = false;
        }
        if (!newlastname.empty()) {
            if (!firstField) query += ", ";
            query += "lastname = '" + newlastname + "'";
            firstField = false;
        }
        if (!newPhoneNumber.empty()) {
            if (!firstField) query += ", ";
            query += "phoneNumber ='" + newPhoneNumber + "'";
            firstField = false;
        }

        query += " WHERE id = " + id + ";";

        if (firstField) {
            cout << "Nie wprowadzono żadnych nowych danych do aktualizacji." << endl;
            continue;
        }

        if (mysql_query(conn, query.c_str())) {
            cout << "Błąd aktualizacji klienta: " << mysql_error(conn) << endl;
        }
        else {
            cout << "Dane klienta zaktualizowane!" << endl;
            logAction("Zaaktualizowano dane o kliencie");
        }

    }
}

void ClientManagement::show() {
    MYSQL* conn = dbConnection.getConnection();
    string query = "SELECT * FROM clients;";


    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd wyświetlania klienta: " << mysql_error(conn) << endl;
    }
    else {
        MYSQL_RES* result = mysql_store_result(conn);
        MYSQL_ROW row;


        while ((row = mysql_fetch_row(result))) {
            cout << "ID: " << row[0] << ", Imię: " << row[1] << ", Nazwisko: " << row[2] << ", Numer telefonu: " << row[3] << endl;
        }


        mysql_free_result(result);
        logAction("Pokazano dane o klientach");
    }
}

// Zarządzanie stanem magazynu
GameManagement::GameManagement(DatabaseConnection& dbConn) : dbConnection(dbConn) {}

void GameManagement::add() {
    MYSQL* conn = dbConnection.getConnection();
    string name, genre, platform;

    cin.ignore(); // Ignoruje pozostały znak nowej linii w buforze po poprzednim cin
    cout << "Przy podawaniu nazwy, gatunku i platformy gry proszę nie podawać polskich znaków.\n";
    cout << "Podaj nazwę gry: ";
    getline(cin, name);
    cout << "Podaj gatunek gry: ";
    getline(cin, genre);
    cout << "Podaj platformę gry: ";
    getline(cin, platform);

    string query = "INSERT INTO Games(name, genre, platform, added_date) VALUES('" + name + "','" + genre + "','" + platform + "', CURRENT_DATE());";

    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd dodawania gry: " << mysql_error(conn) << endl;
    }
    else {
        cout << "Gra została dodana. " << endl;
        logAction("Dodano grę");
    }
}

void GameManagement::remove() {
    cout << "-----Gry na stanie-----" << endl;
    GameManagement::show();
    MYSQL* conn = dbConnection.getConnection();
    string id;

    cout << "Podaj który rekord chcesz usunąć: ";
    cin >> id;

    // Sprawdzenie istnienia gry
    if (!checkIfIdExists(conn, "games", id)) {
        cout << "Gra taka o tym id nie istnieje w bazie danych." << endl;
        return;
    }
    // Sprawdzenie czy gra nie ma relacji
    if (checkIfIdInRelation(conn, "game_id", id)) {
        cout << "Nie można usunąć gry, ponieważ jest powiązana z wypożyczeniami!" << endl;
        return;
    }

    // Usunięcie gry
    string query = "DELETE FROM Games WHERE id = " + id + ";";

    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd usuwania gry: " << mysql_error(conn) << endl;
    }
    else {
        cout << "Gra została usunięta. " << endl;
        logAction("Usunięto grę");
    }
}

void GameManagement::update() {
    MYSQL* conn = dbConnection.getConnection();
    string id, new_name, new_genre, new_platform;
    while (true) {
        cout << "Wpisz '0', żeby wyjść.\n";
        cout << "Wprowadź ID gry do zaktualizowania lub naciśnij 'F' aby wyświetlić listę: ";
        cin >> id;

        if (id == "0") return;
        if (id == "F" or id == "f")
        {
            cout << endl;
            cout << "-----Gry na stanie-----" << endl;
            GameManagement::show();
            continue;
        }
        // Sprawdzenie istnienia gry
        if (!checkIfIdExists(conn, "games", id)) {
            cout << "Gra taka o tym id nie istnieje w bazie danych." << endl;
            return;
            continue;
        }

        // Pobieranie nowych danych
        cin.ignore();
        cout << "Wprowadź Nazwę: ";
        getline(cin, new_name);
        cout << "Podaj Gatunek: ";
        getline(cin, new_genre);
        cout << "Podaj Platformę: ";
        getline(cin, new_platform);

        // Aktualizacja danych gry
        string query = "UPDATE Games SET ";
        bool firstField = true;

        if (!new_name.empty()) {
            query += "name = '" + new_name + "'";
            firstField = false;
        }
        if (!new_genre.empty()) {
            if (!firstField) query += ", ";
            query += "genre = '" + new_genre + "'";
            firstField = false;
        }
        if (!new_platform.empty())
        {
            if (!firstField) query += ", ";
            query += "platform = '" + new_platform + "'";
            firstField = false;
        }
        if (!firstField) {
            query += ", added_date = CURRENT_DATE()";
        }

        query += " WHERE id = " + id + ";";

        if (firstField) {
            cout << "Nie wprowadzono żadnych nowych danych do aktualizacji." << endl;

        }

        if (mysql_query(conn, query.c_str())) {
            cout << "Błąd aktualizacji gry: " << mysql_error(conn) << endl;
        }
        else {
            cout << "Dane gry zaktualizowane!" << endl;
            logAction("Zaaktualizowano dane o grze");
            continue;
        }
        Sleep(1000);
    }
}

void GameManagement::show() {
    MYSQL* conn = dbConnection.getConnection();
    string query = "SELECT * FROM Games;";

    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd wyświetlania gry: " << mysql_error(conn) << endl;
    }
    else {
        MYSQL_RES* result = mysql_store_result(conn);
        MYSQL_ROW row;

        while ((row = mysql_fetch_row(result))) {
            cout << "ID: " << row[0] << ", nazwa: " << row[1] << ", Gatunek: " << row[2] << ", Platforma: " << row[3] << ", Data Dodania: " << row[4] << endl;
        }

        mysql_free_result(result);
        logAction("Pokazano dane o grach");
    }
}

// 
void RentalManagement::LateReturnChecker::displayLateReturns() {
    MYSQL* conn = dbConnection.getConnection();
    MYSQL_RES* res;
    MYSQL_ROW row;

    string query = R"(
        SELECT clients.id, clients.name, games.name AS game_name, rentals.return_date 
        FROM rentals
        JOIN clients ON rentals.client_id = clients.id
        JOIN games ON rentals.game_id = games.id
        WHERE rentals.return_date < CURRENT_DATE;
    )";

    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd podczas wykonywania zapytania: " << mysql_error(conn) << endl;
        return;
    }


    res = mysql_store_result(conn);
    if (!res) {
        cout << "Błąd podczas pobierania wyników: " << mysql_error(conn) << endl;
        return;
    }

    cout << "--- Klienci z przekroczonym terminem zwrotu ---" << endl;
    while ((row = mysql_fetch_row(res))) {
        cout << "ID klienta: " << row[0]
            << ", Imię: " << row[1]
            << ", Gra: " << row[2]
            << ", Data zwrotu: " << row[3] << endl;
    }

    mysql_free_result(res);
    Management::logAction("Utworzono liste klientow z przekroczonym terminem zwrotu");
}

void RentalManagement::showAvailableClientsAndGames() {//metoda używana dla klasy rentals do wyświetlenia dostępnych klientów i gier które nie są wypożyczone na ten moment
    MYSQL* conn = dbConnection.getConnection();
    MYSQL_RES* result;
    MYSQL_ROW row;

    // Wyświetlenie dostępnych klientów
    string queryClients = "SELECT id, name, lastname, phoneNumber FROM clients;";
    if (mysql_query(conn, queryClients.c_str())) {
        cout << "Błąd podczas pobierania klientów: " << mysql_error(conn) << endl;
        return;
    }

    result = mysql_store_result(conn);
    if (!result) {
        cout << "Błąd przechowywania wyników: " << mysql_error(conn) << endl;
        return;
    }

    cout << "--- Dostępni klienci ---" << endl;
    while ((row = mysql_fetch_row(result))) {
        cout << "ID: " << row[0] << ", Imię: " << row[1] << ", Nazwisko: " << row[2] << ", Numer tel: " << row[3] << endl;
    }
    cout << endl;
    mysql_free_result(result);

    // Wyświetlenie dostępnych gier (które nie są wypożyczone)
    string queryGames =
        "SELECT games.id, games.name FROM games "
        "LEFT JOIN rentals ON games.id = rentals.game_id "
        "WHERE rentals.game_id IS NULL;";
    if (mysql_query(conn, queryGames.c_str())) {
        cout << "Błąd podczas pobierania dostępnych gier: " << mysql_error(conn) << endl;
        return;
    }

    result = mysql_store_result(conn);
    if (!result) {
        cout << "Błąd przechowywania wyników: " << mysql_error(conn) << endl;
        return;
    }

    cout << "--- Dostępne gry ---" << endl;
    while ((row = mysql_fetch_row(result))) {
        cout << "ID: " << row[0] << ", Tytuł: " << row[1] << endl;
    }
    cout << endl;
    mysql_free_result(result);
}

void RentalManagement::add() {
    MYSQL* conn = dbConnection.getConnection();
    string client_id, game_id;
    string userChoice, query, incomeType = "Wypozyczenie gry", notes;
    double grossProfit;

    showAvailableClientsAndGames();

    // Wczytanie danych
    cout << "Podaj id klienta: ";
    cin >> client_id;
    if (!checkIfIdExists(conn, "clients", client_id)) {
        cout << "Brak klienta o wskazanym id. Ponów operacje." << endl;
        return;
    }

    cout << "Podaj id gry: ";
    cin >> game_id;
    if (!checkIfIdExists(conn, "games", game_id)) {
        cout << "Brak gry o podanym id. Ponów operacje." << endl;
        return;
    }

    // Sprawdzenie, czy gra jest wypożyczona
    if (mysql_query(conn, ("SELECT COUNT(*) FROM rentals WHERE game_id = " + game_id + ";").c_str())) {
        cout << "Błąd zapytania: " << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        cout << "Błąd przechowywania wyników: " << mysql_error(conn) << endl;
        return;
    }

    int count = atoi(mysql_fetch_row(result)[0]);
    mysql_free_result(result);

    if (count > 0) {
        cout << "Gra jest już wypożyczona!" << endl;
        return;
    }

    // Wybór typu wypożyczenia
    cout << "Podaj typ wypożyczenia (Przedłużony 30 dni - P, Standardowy 15 dni - S): ";
    cin >> userChoice;
    if (userChoice == "P" || userChoice == "p") {
        grossProfit = 40.0;
        notes = "Przedluzenie";
    }
    else if (userChoice == "S" || userChoice == "s") {
        grossProfit = 20.0;
        notes = "Standardowe";
    }
    else {
        cout << "Nieprawidłowy wybór. Anulowanie operacji." << endl;
        return;
    }

    // Wstawienie wypożyczenia
    string dueDate = (userChoice == "P" || userChoice == "p") ? "CURRENT_DATE() + INTERVAL 30 DAY" : "CURRENT_DATE() + INTERVAL 15 DAY";
    query = "INSERT INTO rentals(client_id, game_id, rental_date, return_date) VALUES(" +
        client_id + ", " + game_id + ", CURRENT_DATE(), " + dueDate + ");";

    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd dodawania wypożyczenia: " << mysql_error(conn) << endl;
        return;
    }

    // Pobranie ostatniego ID wypożyczenia
    if (mysql_query(conn, "SELECT LAST_INSERT_ID();")) {
        cout << "Błąd pobierania ID wypożyczenia: " << mysql_error(conn) << endl;
        return;
    }

    result = mysql_store_result(conn);
    int rentalId = atoi(mysql_fetch_row(result)[0]);
    mysql_free_result(result);

    cout << "Dodano nowe wypożyczenie z ID: " << rentalId << endl;

    // Dodanie wpisu do tabeli income
    query = "INSERT INTO income (income_type, gross_profit, notes, income_date) VALUES ('" + incomeType + "', " + to_string(grossProfit) + ", '" + notes + "', CURRENT_DATE());";

    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd podczas dodawania rekordu: " << mysql_error(conn) << endl;
    }
    else {
        logAction("Dodano wypożyczenie");
    }

}

void RentalManagement::remove() {
    cout << "-----Lista obecnych wypożyczeń-----" << endl;
    RentalManagement::show();
    MYSQL* conn = dbConnection.getConnection();
    string rentalId;
    string gameCondition;

    // Pobierz ID wypożyczenia od użytkownika
    cout << "Podaj ID wypożyczenia, które chcesz usunąć: ";
    cin >> rentalId;

    // sprawdzenie metodą czy coś istnieje
    if (!checkIfIdExists(conn, "rentals", rentalId)) {
        cout << "Wypożyczenie o podanym ID nie istnieje. Ponów operacje." << endl;
        return;
    }

    // Pobierz stan fizyczny gry
    cout << "Czy stan fizyczny gry został naruszony? (Nie - N, Uszkodzenie - U, Całkowite zniszczenie - C): ";
    cin >> gameCondition;

    bool conditionFeeApplicable = false;
    bool overdueFeeApplicable = false;
    double conditionFee = 0.0;
    double overdueFee = 0.0;
    string conditionNotes;

    // Sprawdź i nalicz opłatę za stan fizyczny gry
    if (gameCondition == "N" || gameCondition == "n") {
        conditionNotes = "Brak uszkodzen";
    }
    else if (gameCondition == "U" || gameCondition == "u") {
        conditionFee = 20.0;
        conditionNotes = "Uszkodzenie gry";
        conditionFeeApplicable = true;
    }
    else if (gameCondition == "C" || gameCondition == "c") {
        conditionFee = 50.0;
        conditionNotes = "Calkowite zniszczenie gry";
        conditionFeeApplicable = true;
    }
    else {
        cout << "Nieprawidłowy wybór. Anulowanie operacji." << endl;
        return;
    }

    // Sprawdź, czy wypożyczenie przekroczyło termin zwrotu
    string overdueCheckQuery = "SELECT DATEDIFF(CURRENT_DATE(), return_date) FROM rentals WHERE id = " + rentalId + ";";
    if (mysql_query(conn, overdueCheckQuery.c_str())) {
        cout << "Błąd sprawdzania terminu zwrotu: " << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        cout << "Błąd przechowywania wyników: " << mysql_error(conn) << endl;
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    int overdueDays = row ? atoi(row[0]) : 0;
    mysql_free_result(result);

    if (overdueDays > 0) {
        overdueFee = overdueDays * 0.50; // 50 groszy za dzień opóźnienia
        overdueFeeApplicable = true;
    }

    // Usuń wypożyczenie z tabeli rentals
    string deleteQuery = "DELETE FROM rentals WHERE id = " + rentalId + ";";
    if (mysql_query(conn, deleteQuery.c_str())) {
        cout << "Błąd usuwania wypożyczenia: " << mysql_error(conn) << endl;
        return;
    }
    else {
        logAction("Usunięto wypożyczenie");
    }

    // Dodaj wpis do tabeli income za stan gry (jeśli dotyczy)
    if (conditionFeeApplicable) {
        string conditionIncomeQuery = "INSERT INTO income (income_type, gross_profit, notes, income_date) VALUES ('Dodatkowa oplata', " +
            to_string(conditionFee) + ", '" + conditionNotes + "', CURRENT_DATE());";
        if (mysql_query(conn, conditionIncomeQuery.c_str())) {
            cout << "Błąd dodawania wpisu za stan gry do income: " << mysql_error(conn) << endl;
            return;
        }
    }

    // Dodaj wpis do tabeli income za opóźnienie (jeśli dotyczy)
    if (overdueFeeApplicable) {
        string overdueIncomeQuery = "INSERT INTO income (income_type, gross_profit, notes, income_date) VALUES ('Dodatkowa oplata', " +
            to_string(overdueFee) + ", 'Oddano po terminie', CURRENT_DATE());";
        if (mysql_query(conn, overdueIncomeQuery.c_str())) {
            cout << "Błąd dodawania wpisu za opóźnienie do income: " << mysql_error(conn) << endl;
            return;
        }
    }

    cout << "Wypożyczenie zostało usunięte.";
    if (conditionFeeApplicable || overdueFeeApplicable) {
        cout << " Dodatkowe opłaty zostały naliczone i dodane do tabeli income." << endl;
        logAction("Dodano nowe informacje o opłatach");
    }
    else {
        cout << " Żadne dodatkowe opłaty nie zostały naliczone." << endl;
    }
}

void RentalManagement::update() {
    cout << "-----Obecne wypożyczenia-----" << endl;
    RentalManagement::show();
    MYSQL* conn = dbConnection.getConnection();
    string id, new_clientid, new_gameid;

    showAvailableClientsAndGames();
    cout << "Wprowadź ID wypożyczenia do aktualizacji lub wprowadź 0 aby wyjść: ";
    cin >> id;
    if (id == "0") return;

    if (!checkIfIdExists(conn, "rentals", id)) {
        cout << "Wypożyczenie o podanym ID nie istnieje. Ponów operacje." << endl;
        return;
    }

    cout << "Wprowadź ID klienta lub naciśnij Enter, jeżeli chcesz pominąć: ";
    cin.ignore(); // Ignorowanie pozostałości po poprzednim wejściu
    getline(cin, new_clientid);

    cout << "Podaj ID gry lub naciśnij Enter, jeżeli chcesz pominąć: ";
    getline(cin, new_gameid);

    // Sprawdzanie poprawności nowego klienta, jeśli podano
    if (!new_clientid.empty() && !checkIfIdExists(conn, "clients", new_clientid)) {
        cout << "Brak klienta o podanym ID." << endl;
        return;
    }

    // Sprawdzanie poprawności nowej gry, jeśli podano
    if (!new_gameid.empty() && !checkIfIdExists(conn, "games", new_gameid)) {
        cout << "Brak gry o podanym ID." << endl;
        return;
    }

    // Sprawdzenie, czy gra nie jest już wypożyczona
    if (!new_gameid.empty()) {
        string checkGameQuery = "SELECT COUNT(*) FROM rentals WHERE game_id = '" + new_gameid + "';";
        if (mysql_query(conn, checkGameQuery.c_str())) {
            cout << "Błąd zapytania: " << mysql_error(conn) << endl;
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (!result) {
            cout << "Błąd przechowywania wyników: " << mysql_error(conn) << endl;
            return;
        }

        int count = atoi(mysql_fetch_row(result)[0]);
        mysql_free_result(result);

        if (count > 0) {
            cout << "Gra jest już wypożyczona!" << endl;
            return;
        }
    }

    // Budowanie zapytania SQL
    string query = "UPDATE Rentals SET ";
    bool firstField = false;

    if (!new_clientid.empty()) {
        query += "id_klienta = '" + new_clientid + "'";
        firstField = true;
    }

    if (!new_gameid.empty()) {
        if (firstField) query += ", ";
        query += "id_gry = '" + new_gameid + "'";
        firstField = true;
    }

    // Jeśli nie podano żadnych danych do aktualizacji
    if (!firstField) {
        cout << "Nie wprowadzono żadnych nowych danych do aktualizacji." << endl;
        return;
    }

    query += " WHERE id = " + id + ";";

    // Wykonanie zapytania
    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd aktualizacji danych o wypożyczeniu: " << mysql_error(conn) << endl;
    }
    else {
        cout << "Dane wypożyczenia zaktualizowane!" << endl;
        logAction("Zaaktualizowano dane o wypożyczeniu");
    }
}

void RentalManagement::show() {
    MYSQL* conn = dbConnection.getConnection();
    string query = "SELECT * FROM Rentals;";

    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd wyświetlania wypożyczenia: " << mysql_error(conn) << endl;
    }
    else {
        MYSQL_RES* result = mysql_store_result(conn);
        MYSQL_ROW row;

        while ((row = mysql_fetch_row(result))) {
            cout << "ID: " << row[0] << ", id klienta: " << row[1] << ", id gry: " << row[2] << ", Data wypozyczenia: " << row[3] << ", Data zwrotu: " << row[4] << endl;
        }
        mysql_free_result(result);
        logAction("Pokazano dane o wypożyczeniach");
    }
}

Reporting::Reporting(DatabaseConnection& dbConnection) : dbConnection(dbConnection) {}

// Metoda odpowiedzialna za generowanie raportu z wypożyczeniami
void Reporting::generateRentalReport() {
    MYSQL* conn = dbConnection.getConnection();
    MYSQL_RES* result;
    MYSQL_ROW row;

    string query = "SELECT * FROM Rentals";

    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd zapytania: " << mysql_error(conn) << endl;
        return;
    }

    result = mysql_store_result(conn);
    if (!result) {
        cout << "Błąd przechowywania wyników: " << mysql_error(conn) << endl;
        return;
    }

    ofstream reportFile("raport_wypozyczen.txt");
    if (!reportFile) {
        cout << "Nie udało się otworzyć pliku do zapisu." << endl;
        mysql_free_result(result);
        return;
    }

    reportFile << "Raport aktualnych wypożyczeń:\n\n";

    while ((row = mysql_fetch_row(result))) {
        reportFile << "ID Wypożyczenia: " << row[0]
            << ", ID Klienta: " << row[1]
            << ", ID Gry: " << row[2]
            << ", Data Wypożyczenia: " << row[3]
            << ", Data zwrotu: " << row[4] << "\n";
    }

    reportFile.close();
    mysql_free_result(result);

    cout << "Raport został wygenerowany i zapisany w pliku: raport_wypozyczen.txt" << endl;
    Management::logAction("Utworzono raport wypozyczeń");
}

//raportowanie wyników finansowych
void Reporting::generateProfitReport() {
    MYSQL* conn = dbConnection.getConnection();
    MYSQL_RES* result;
    MYSQL_ROW row;
    string startDate, endDate;

    cout << "Podaj przedział czasowy od do w formacie (YYYY.MM.DD)" << endl;
    cout << "Początkowa data okresu: ";
    cin >> startDate;
    cout << "Końcowa data okresu: ";
    cin >> endDate;

    string query = "SELECT income_type, "
        "notes, "
        "SUM(gross_profit) AS total_profit, "
        "COUNT(*) AS transaction_count "
        "FROM income "
        "WHERE income_date BETWEEN '" + startDate + "' AND '" + endDate + "' "
        "GROUP BY income_type, notes "
        "ORDER BY income_type, total_profit DESC;";

    if (mysql_query(conn, query.c_str())) {
        cout << "Błąd zapytania: " << mysql_error(conn) << endl;
        return;
    }

    result = mysql_store_result(conn);
    if (!result) {
        cout << "Błąd przechowywania wyników: " << mysql_error(conn) << endl;
        return;
    }

    ofstream reportFile("raport_transakcji.txt");
    if (!reportFile) {
        cout << "Nie udało się otworzyć pliku do zapisu." << endl;
        mysql_free_result(result);
        return;
    }

    reportFile << "--- RAPORT TRANSAKCJI ---\n";
    reportFile << "Okres: od " << startDate << " do " << endDate << "\n\n";

    string currentIncomeType = "";
    while ((row = mysql_fetch_row(result))) {
        string incomeType = row[0] ? row[0] : "Nieznany typ"; // Typ zarobku
        string note = row[1] ? row[1] : "Brak uwag"; // notes
        double totalProfit = row[2] ? atof(row[2]) : 0.0; // Całkowity zysk
        int transactionCount = row[3] ? atoi(row[3]) : 0; // Liczba transakcji

        if (incomeType != currentIncomeType) {
            if (!currentIncomeType.empty()) {
                reportFile << "\n"; // Dodaj odstęp między typami zarobków
            }
            reportFile << "Typ: " << incomeType << "\n";
            currentIncomeType = incomeType;
        }

        reportFile << "  notes: " << note
            << ", Zysk: " << totalProfit
            << " PLN, Liczba transakcji: " << transactionCount << "\n";
    }

    reportFile.close();
    mysql_free_result(result);

    cout << "Raport został zapisany w pliku: raport_transakcji.txt" << endl;
    Management::logAction("Utworzono raport transakcji");
}

// metoda sprawdza czy któryś z klientów przekroczył dzień oddania gry

Menu::Menu(ClientManagement& clientManagement, GameManagement& gameManagement, RentalManagement& rentalManagement, Reporting& reporting)
    : clientManagement(clientManagement), gameManagement(gameManagement), rentalManagement(rentalManagement), reporting(reporting) {
}

bool Menu::validateInput(const string& input) {
    for (char c : input) {
        if (!isdigit(c)) {
            return false;
        }

    }
    return true;
}

void Menu::displayMenu() {
    string inputChoice;
    bool start = true;

    while (start) {
        system("cls");
        cout << "---MENU GŁÓWNE---" << endl;
        cout << "1. Zarządzanie klientami." << endl;
        cout << "2. Zarządzanie grami na stanie." << endl;
        cout << "3. Zarządzanie wypożyczeniami." << endl;
        cout << "4. Raportowanie." << endl;
        cout << "5. Zakończ." << endl;
        cout << "Wybierz opcję: ";
        cin >> inputChoice;

        if (!validateInput(inputChoice)) {
            cout << "Wprowadzono niezgodny znak!" << endl;
        }

        try {
            int choice = stoi(inputChoice); // Konwersja ciągu na liczbę

            switch (choice) {
            case 1:
                system("cls");
                clientManagementMenu();
                break;
            case 2:
                system("cls");
                gameManagementMenu();
                break;
            case 3:
                system("cls");
                rentalManagementMenu();
                break;
            case 4:
                system("cls");
                reportMenu();
                break;
            case 5:
                start = false;
                Management::logAction("Zakończono działanie programu");
                break;
            default:
                cout << "Nieprawidłowa opcja!" << endl;
                system("pause");
            }
        }
        catch (const invalid_argument&) {
            cout << "Wprowadź wyłącznie cyfry!" << endl;
            system("pause");
        }
        catch (const out_of_range&) {
            cout << "Opcja jest poza dozwolonym zakresem!" << endl;
            system("pause");
        }
    }
}

void Menu::clientManagementMenu() {
    string inputChoice;
    cout << "---ZARZĄDZANIE KLIENTAMI---" << endl;
    cout << "1. Dodaj klienta." << endl;
    cout << "2. Usuń klienta." << endl;
    cout << "3. Zaktualizuj dane klienta." << endl;
    cout << "4. Profile klientów." << endl;
    cout << "5. Wróć do menu głównego." << endl;
    cout << "Wybierz opcję: ";
    cin >> inputChoice;


    if (!validateInput(inputChoice)) {
        cout << "Wprowadzono niezgodny znak!" << endl;
        system("pause");
    }

    try {
        int choice = stoi(inputChoice); // Konwersja ciągu na liczbę

        Management* management = nullptr;

        // Tworzymy obiekt klasy ClientManagement tylko dla opcji 1-4
        if (choice >= 1 && choice <= 4) {
            management = new ClientManagement(dbConnection);
        }

        switch (choice) {
        case 1:
            system("cls");
            management->add(); //polimorfizm
            Management::back();
            break;
        case 2:
            system("cls");
            clientManagement.remove();
            Management::back();
            break;
        case 3:
            system("cls");
            clientManagement.update();
            Management::back();
            break;
        case 4:
            system("cls");
            clientManagement.show();
            Management::back();
            break;
        case 5:
            break;
        default:
            cout << "Nieprawidłowa opcja!" << endl;
            system("pause");
        }
    }
    catch (const invalid_argument&) {
        cout << "Wprowadź wyłącznie cyfry!" << endl;
        system("pause");
    }
    catch (const out_of_range&) {
        cout << "Opcja jest poza dozwolonym zakresem!" << endl;
        system("pause");
    }
}


void Menu::gameManagementMenu() {
    string inputChoice;
    cout << "---ZARZĄDZANIE MAGAZYNEM---" << endl;
    cout << "1. Dodaj grę wideo." << endl;
    cout << "2. Usuń grę." << endl;
    cout << "3. Zaktualizuj dane." << endl;
    cout << "4. Gry na stanie." << endl;
    cout << "5. Wróć do menu głównego." << endl;
    cout << "Wybierz opcję: ";
    cin >> inputChoice;


    if (!validateInput(inputChoice)) {
        cout << "Wprowadzono niezgodny znak!" << endl;
        system("pause");
    }

    try {
        int choice = stoi(inputChoice); // Konwersja ciągu na liczbę

        switch (choice) {
        case 1:
            system("cls");
            gameManagement.add();
            Management::back();
            break;
        case 2:
            system("cls");
            gameManagement.remove();
            Management::back();
            break;
        case 3:
            system("cls");
            gameManagement.update();
            Management::back();
            break;
        case 4:
            system("cls");
            gameManagement.show();
            Management::back();
            break;
        case 5:
            break;
        default:
            cout << "Nieprawidłowa opcja!" << endl;
        }
    }
    catch (const invalid_argument&) {
        cout << "Wprowadź wyłącznie cyfry!" << endl;
        system("pause");
    }
    catch (const out_of_range&) {
        cout << "Opcja jest poza dozwolonym zakresem!" << endl;
        system("pause");
    }
}

void Menu::rentalManagementMenu() {
    string inputChoice;
    cout << "---ZARZĄDZANIE WYPOŻYCZENIAMI---" << endl;
    cout << "1. Dodaj wypożyczenie." << endl;
    cout << "2. Usuń wypożyczenie." << endl;
    cout << "3. Zaktualizowanie danych o wypożyczeniach." << endl;
    cout << "4. Aktualne wypożyczenia." << endl;
    cout << "5. Sprawdź opóżnione zwroty." << endl;
    cout << "6. Wróć do menu głównego." << endl;
    cout << "Wybierz opcję: ";
    cin >> inputChoice;

    if (!validateInput(inputChoice)) {
        cout << "Wprowadzono niezgodny znak!" << endl;
        system("pause");
    }

    try {
        int choice = stoi(inputChoice); // Konwersja ciągu na liczbę

        switch (choice) {
        case 1:
            system("cls");
            rentalManagement.add();
            Management::back();
            break;
        case 2:
            system("cls");
            rentalManagement.remove();
            Management::back();
            break;
        case 3:
            system("cls");
            rentalManagement.update();
            Management::back();
            break;
        case 4:
            system("cls");
            rentalManagement.show();
            Management::back();
            break;
        case 5:
            system("cls");
            feeManagementMenu();
            Management::back;
            break;
        case 6:
            break;
        default:
            cout << "Nieprawidłowa opcja!" << endl;
        }
    }
    catch (const invalid_argument&) {
        cout << "Wprowadź wyłącznie cyfry!" << endl;
        system("pause");
    }
    catch (const out_of_range&) {
        cout << "Opcja jest poza dozwolonym zakresem!" << endl;
        system("pause");
    }
}

void Menu::feeManagementMenu() {
    string inputChoice;
    cout << "---ZARZĄDZANIE OPŁATAMI---" << endl;
    cout << "1. Przekroczone terminy zwrotu." << endl;
    cout << "2. Powrót do zarządzania wypożyczeniami." << endl;
    cout << "Wybierz opcję: ";
    cin >> inputChoice;

    if (!validateInput(inputChoice)) {
        cout << "Wprowadzono niezgodny znak!" << endl;
        system("pause");
    }

    try {
        int choice = stoi(inputChoice); // Konwersja ciągu na liczbę

        switch (choice) {
        case 1:
            rentalManagement.getLateReturnChecker().displayLateReturns();
            Management::back();
            break;
        case 2:
            break; // Powrót do menu zarządzania wypożyczeniami
        default:
            cout << "Nieprawidłowa opcja. Spróbuj ponownie." << endl;
            break;
        }
    }
    catch (const invalid_argument&) {
        cout << "Wprowadź wyłącznie cyfry!" << endl;
        system("pause");
    }
    catch (const out_of_range&) {
        cout << "Opcja jest poza dozwolonym zakresem!" << endl;
        system("pause");
    }
}

void Menu::reportMenu() {
    string inputChoice;
    cout << "---RAPORTOWANIE---" << endl;
    cout << "1. Generuj raport wypożyczeń." << endl;
    cout << "2. Generuj raport zysku z wypożyczeń." << endl;
    cout << "3. Wróć do menu głównego." << endl;
    cout << "Wybierz opcję: ";
    cin >> inputChoice;

    if (!validateInput(inputChoice)) {
        cout << "Wprowadzono niezgodny znak!" << endl;
        system("pause");
    }

    try {
        int subChoice = stoi(inputChoice); // Konwersja ciągu na liczbę
        switch (subChoice) {
        case 1:
            system("cls");
            reporting.generateRentalReport();
            Management::back();
            break;
        case 2:
            system("cls");
            reporting.generateProfitReport();
            Management::back();
            break;
        case 3:
            system("cls");
            break;
        default:
            cout << "Niepoprawny wybór, spróbuj ponownie." << endl;
            break;
        }
    }
    catch (const invalid_argument&) {
        cout << "Wprowadź wyłącznie cyfry!" << endl;
        system("pause");
    }
    catch (const out_of_range&) {
        cout << "Opcja jest poza dozwolonym zakresem!" << endl;
        system("pause");
    }
}