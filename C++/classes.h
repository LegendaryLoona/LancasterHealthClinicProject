#include "crow.h"
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <map>
#include <sqlite3.h>
using namespace std;

class MedicalHistory {
public:                    
    string doctor;
    tm date;
    string detail;
    // Constructor
    MedicalHistory(string doctor, tm& date, string detail)
        : doctor(doctor), date(date), detail(detail) {}
    // JSON-like format
    string to_json() const {
        char mid[50];
        strftime (mid, sizeof(mid), "%Y-%m-%d %H:%M", &date);
        return "{ \"doctor\": " + doctor +
               "\", \"date\": \"" + mid +
               "\", \"detail\": \"" + detail + "\"}";
    }
};

class Bill {
public:
    string billid;
    string amount;
    tm date;
    string detail;
    bool insurance_claim;
    // Constructor
    Bill(string billid, string amount, tm& date, string detail)
        : billid(billid), amount(amount), date(date), detail(detail), insurance_claim(false) {}

    void insurance_submit() {
        insurance_claim = true;
    }

    // JSON-like format
    string to_json() const {
        char mid[50];
        strftime (mid, sizeof(mid), "%Y-%m-%d %H:%M", &date);
        return "{ \"billid\": " + billid +
               "\", \"amount\": " + amount +
               "\", \"date\": \"" + mid +
               "\", \"detail\": \"" + detail + 
               "\", \"insurance_status\": " + (insurance_claim ? "true" : "false") + "\" }";
    }
};

class Patient {
public:
    int id;                    
    std::string name;
    int age;
    std::string insurance;
    vector<MedicalHistory> medicalhistory;
    vector<Bill> bill;
        struct Prescription {
        std::string doctor_name;
        std::string date_time; 
    };
    std::map<std::string, Prescription> prescriptions;

    // Constructor
    Patient(int id, std::string name, int age, std::string insurance)
        : id(id), name(name), age(age), insurance(insurance) {}

    void add_prescription(const std::string& prescription_name, const std::string& doctor_name) {
        std::time_t t = std::time(nullptr);
        std::tm* now = std::localtime(&t);
        std::ostringstream time_stream;
        time_stream << std::put_time(now, "%Y-%m-%d %H:%M:%S");
        prescriptions[prescription_name] = {doctor_name, time_stream.str()};
    }

    std::string prescriptions_to_json() const {
    std::string json = "{";
    for (auto it = prescriptions.begin(); it != prescriptions.end(); ++it) {
        json += "\"" + it->first + "\": { \"doctor\": \"" + it->second.doctor_name +
                "\", \"date_time\": \"" + it->second.date_time + "\" }";
        if (std::next(it) != prescriptions.end()) {
            json += ", "; 
        }
    }
    json += "}"; 
    return json;
    }

    void medicalhistory_add(const MedicalHistory& mh_new) {
        medicalhistory.push_back(mh_new);
    }
    const vector<MedicalHistory>& medicalhistory_get() {
        return medicalhistory;
    }

    void bill_add(const Bill& bill_new) {
        bill.push_back(bill_new);
    }

    const vector<Bill>& bill_get() {
        return bill;
    }

    // Method to convert Doctor details to JSON-like format
    string to_json() const {
        string json = "{ \"id\": " + std::to_string(id) +
                    "\", \"name\": \"" + name +
                    "\", \"age\": \"" + std::to_string(age) +
                    ", \"insurance\": \"" + insurance +
                    "\", \"Prescriptions\": \"" + prescriptions_to_json() +
                    "\", \"MedicalHistory\": [";
        for (int i = 0; i < medicalhistory.size(); ++i) {
            json += medicalhistory[i].to_json();
            if (i < medicalhistory.size() - 1) {
                json += ", ";
            }
        }
        json += "], \"Bill\": [";
        for (int i = 0; i < bill.size(); ++i) {
            json += bill[i].to_json();
            if (i < bill.size() - 1) {
                json += ", ";
            }
        }
        json += "] }";
        return json;
    }
};



class Doctor {
public:
    int id;                    
    std::string name;           
    std::string specialization; 
    std::string start_time;   
    std::string end_time; 
    struct std::tm start_time_tm{};
    struct std::tm end_time_tm{};
    std::map<std::string, std::string> appointments;

    // Constructor
    Doctor(int id, std::string name, std::string specialization, std::string start_time, std::string end_time)
        : id(id), name(name), specialization(specialization), start_time(start_time), end_time(end_time) {
        parse_time(start_time, start_time_tm);
        parse_time(end_time, end_time_tm);
        generate_appointments();
    }

    // Parse time string into a struct tm
    void parse_time(const std::string& time_str, struct std::tm& time_tm) {
        std::istringstream ss(time_str);
        ss >> std::get_time(&time_tm, "%R");
        if (ss.fail()) {
            std::cerr << "Error parsing time: " << time_str << std::endl;
        }
    }

    // Generate appointments in 30-minute intervals
    void generate_appointments() {
        struct std::tm current_time = start_time_tm;

        while (std::mktime(&current_time) <= std::mktime(&end_time_tm)) {
            std::ostringstream time_stream;
            time_stream << std::put_time(&current_time, "%H:%M");
            appointments[time_stream.str()] = "available";

            current_time.tm_min += 30;
            if (current_time.tm_min >= 60) {
                current_time.tm_hour++;
                current_time.tm_min -= 60;
            }
        }
    }

    // Convert appointments to JSON
    std::string appointments_to_json() const {
        std::string json = "{";
        for (auto it = appointments.begin(); it != appointments.end(); ++it) {
            json += "\"" + it->first + "\": \"" + it->second + "\"";
            if (std::next(it) != appointments.end()) {
                json += ", ";
            }
        }
        json += "}";
        return json;
    }

    // Convert Doctor details to JSON
    std::string to_json() const {
        return "{ \"id\": " + std::to_string(id) +
               ", \"name\": \"" + name +
               "\", \"specialization\": \"" + specialization +
               "\", \"availability\": \"" + start_time + " - " + end_time +
               "\" }";
    }
};

void createTables(sqlite3* db) {
    int rc = sqlite3_open("hospital.db", &db);
    if (rc) {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        return;
    }
    cout << "Opened database successfully!" << endl;
    const char* sql = R"(
    CREATE TABLE IF NOT EXISTS patients (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        age INTEGER NOT NULL,
        insurance TEXT NOT NULL
        );

    CREATE TABLE IF NOT EXISTS doctors (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        specialization TEXT NOT NULL,
        start_time TEXT NOT NULL,
        end_time TEXT NOT NULL
        );

    CREATE TABLE IF NOT EXISTS appointments (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        patient_id INTEGER NOT NULL,
        doctor_id INTEGER NOT NULL,
        time TEXT NOT NULL,
        FOREIGN KEY(patient_id) REFERENCES patients(id),
        FOREIGN KEY(doctor_id) REFERENCES doctors(id)
        );
    )";

    char* errMsg = 0;
    rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    } else {
        cout << "Tables created successfully!" << endl;
    }
}

void executeSQL(sqlite3 *db, const std::string &sql) {
    char *errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL Error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}
std::vector<Doctor> fetchDoctors(sqlite3 *db) {
    std::vector<Doctor> doctors;
    const char *sql = "SELECT id, name, specialization, start_time, end_time FROM doctors;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing SQL statement: " << sqlite3_errmsg(db) << std::endl;
        return doctors;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        std::string specialty = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        std::string work_start = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        std::string work_end = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));

        doctors.emplace_back(id, name, specialty, work_start, work_end);
    }

    sqlite3_finalize(stmt);
    return doctors;
}

// Fetch all patients from the database
std::vector<Patient> fetchPatients(sqlite3 *db) {
    std::vector<Patient> patients;
    const char *sql = "SELECT id, name, age, insurance FROM patients;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing SQL statement: " << sqlite3_errmsg(db) << std::endl;
        return patients;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        int age = sqlite3_column_int(stmt, 2);
        std::string insurance = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));

        patients.emplace_back(id, name, age, insurance);
    }

    sqlite3_finalize(stmt);
    return patients;
}

