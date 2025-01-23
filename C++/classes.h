#include "crow.h"
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <map>
using namespace std;

class Prescription {
public:
    string patient;
    string doctor;
    tm date;
    string medname;
    string amount;
    string duration;
    //Constructor
    Prescription(string patient, string doctor, tm& date, string medname, string amount, string duration)
        : patient(patient), doctor(doctor), date(date), medname(medname), amount(amount), duration(duration) {}
    // JSON-like format
    string to_json() const {
        char mid[50];
        strftime (mid, sizeof(mid), "%Y-%m-%d %H:%M", &date);
        return  "{ \"patient\": " + patient +
                "\", \"doctor\": \"" + doctor +
                "\", \"date\": \"" + mid +
                "\", \"medname\": \"" + medname +
                "\", \"amount\": \"" + amount +
                "\", \"duration\": \"" + duration + "\"}";
    }

};

class MedicalHistory {
public:                    
    string doctor;
    tm date;
    string detail;
    vector<Prescription> prescription;
    // Constructor
    MedicalHistory(string doctor, tm& date, string detail)
        : doctor(doctor), date(date), detail(detail) {}
    
    void prescription_add(const Prescription& pres_new) {
        prescription.push_back(pres_new);
    }
    const vector<Prescription>& prescription_get() {
        return prescription;
    }
    // JSON-like format
    string to_json() const {
        char mid[50];
        strftime (mid, sizeof(mid), "%Y-%m-%d %H:%M", &date);
        string json =  "{ \"doctor\": " + doctor +
               "\", \"date\": \"" + mid +
               "\", \"detail\": \"" + detail +
               "\", \"Prescriptions\": [";
        for (size_t i = 0; i < prescription.size(); ++i) {
            json += prescription[i].to_json();
            if (i < prescription.size() - 1) {
                json += ", ";
            }
        } 
        json += "]}";
        return json;      
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

class Supply {
public:
    int id;
    std::string name;
    int num;
    int min_num;

    // Constructor
    Supply(int id, std::string name, int num, int min_num)
        : id(id), name(name), num(num), min_num(min_num) {}

    // JSON-like format
    std::string to_json() const {
        return "{ \"id\": " + std::to_string(id) +
               ", \"name\": \"" + name +
               "\", \"quantity\": " + std::to_string(num) +
               ", \"minimum value\": " + std::to_string(min_num) +
               " }";
    }
    // Stock low
    bool track() const {
        return num < min_num;
    }
};

class Patient {
public:
    int id;                    
    std::string name;
    int age;
    vector<MedicalHistory> medicalhistory;
    vector<Bill> bill;
        struct Prescription {
        std::string doctor_name;
        std::string date_time; 
    };
    std::map<std::string, Prescription> prescriptions;

    // Constructor
    Patient(int id, std::string name, int age)
        : id(id), name(name), age(age) {}

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

    // Method to convert Patient details to JSON-like format
    string to_json() const {
        string json = "{ \"id\": " + std::to_string(id) +
                    "\", \"name\": \"" + name +
                    "\", \"age\": \"" + std::to_string(age) +
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
