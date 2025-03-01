#include <iostream>
#include <mysql.h>
#include <string>

using namespace std;

// Class to manage the database connection
class DatabaseConnection {
private:
    MYSQL* connection_;
public:
    DatabaseConnection(const string& host = "localhost",
        const string& user = "root",
        const string& password = "yourpassword",
        const string& database = "wypozyczalnia",
        unsigned int port = 3306);
    ~DatabaseConnection();
    MYSQL* getConnection() const;
};

// Abstract class for database operations
class Management {
public:
    virtual void add() = 0;
    virtual void remove() = 0;
    virtual void update() = 0;
    virtual void show() = 0;
    static void logAction(const string& action);
    static void back();
    virtual bool checkIfIdExists(MYSQL* conn, const string& tableName, string id);
    virtual bool checkIfIdInRelation(MYSQL* conn, const string& column, const string& id);

    virtual ~Management() = default;
};

// Class to manage clients
class ClientManagement : public Management {
private:
    DatabaseConnection& dbConnection;
public:
    ClientManagement(DatabaseConnection& dbConn);
    void add() override;
    void remove() override;
    void update() override;
    void show() override;
};

// Class to manage games
class GameManagement : public Management {
private:
    DatabaseConnection& dbConnection;
public:
    GameManagement(DatabaseConnection& dbConn);
    void add() override;
    void remove() override;
    void update() override;
    void show() override;
};


// Class to manage rentals
class RentalManagement : public Management {
private:
    DatabaseConnection& dbConnection;
    // klasa wewnętrzna od zarządzania opłatami oraz pokazywaniem klientów s
    class LateReturnChecker {
    private:
        DatabaseConnection& dbConnection;
    public:
        LateReturnChecker(DatabaseConnection& dbConn)
            : dbConnection(dbConn) {
        }

        void displayLateReturns();// Metoda sprawdzająca i wyświetlająca klientów, którzy nie zwrócili gier na czas

    };

    LateReturnChecker lateReturnChecker;
public:
    RentalManagement(DatabaseConnection& dbConn) : dbConnection(dbConn), lateReturnChecker(dbConn) {}
    void add() override;
    void remove() override;
    void update() override;
    void show() override;
    void showAvailableClientsAndGames();

    LateReturnChecker& getLateReturnChecker() { return lateReturnChecker; }
};

class Reporting {
private:
    DatabaseConnection& dbConnection;
public:
    Reporting(DatabaseConnection& dbConnection);
    void generateRentalReport();
    void generateProfitReport();
};

// Menu class
class Menu {
private:
    DatabaseConnection dbConnection; // Deklaracja połączenia z bazą danych
    ClientManagement& clientManagement;// referencje do konkretnych kategorii zarządzania
    GameManagement& gameManagement;
    RentalManagement& rentalManagement;
    Reporting& reporting;

    bool validateInput(const string& input);

public:
    Menu(ClientManagement& clientManagement, GameManagement& gameManagement, RentalManagement& rentalManagement, Reporting& reporting);
    //Menu(); // Konstruktor
    void displayMenu();
    void clientManagementMenu();
    void gameManagementMenu();
    void rentalManagementMenu();
    void feeManagementMenu();
    void reportMenu();
};

