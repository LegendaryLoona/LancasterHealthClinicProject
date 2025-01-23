#include "crow.h"
//#include "crow_all.h"
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <map>
#include <algorithm>
#include"classes.h"
#include <sqlite3.h>
using namespace std;

int main() {
    crow::SimpleApp app;

    sqlite3* db;
    if (sqlite3_open("hospital.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    createTables(db);
    const std::string insertPatientsSQL = R"(
    INSERT OR REPLACE INTO Patients (id, name, age) VALUES
    (1, 'John Hopf', 55),
    (2, 'Allison Wern', 44),
    (3, 'Sir Munhausen', 33);
    )";
    executeSQL(db, insertPatientsSQL);
    const std::string insertDoctorsSQL = R"(
    INSERT OR REPLACE INTO Doctors (id, name, specialization, start_time, end_time) VALUES
    (1, "Dr. NotAlice Smith", "Cardiology", "9:00", "18:00"),
    (2, "Dr. Bob Jones", "Neurology", "9:00", "18:00"),
    (3, "Dr. Carol White", "Pediatrics", "9:00", "18:00");
    )";
    executeSQL(db, insertDoctorsSQL);
    std::vector<std::string> reminds;
    std::vector<Patient> patients = fetchPatients(db);
    std::vector<Doctor> doctors = fetchDoctors(db);
    std::vector<Supply> supply = fetchSupply(db);

    sqlite3_close(db);

    // Default route
    CROW_ROUTE(app, "/")([]() {
        return "Welcome to the appointment booking system";
    });

    // Route: Get all supply
    CROW_ROUTE(app, "/supply")([&supply]() {
        std::string response = "[";
        for (size_t i = 0; i < supply.size(); ++i) {
            response += supply[i].to_json();
            if (i != supply.size() - 1) response += ", ";
        }
        response += "]";
        return crow::response(response);
    });

    //Route: Supply entry
    CROW_ROUTE(app, "/supply_add")([&supply](const crow::request& req) {
        auto name = req.url_params.get("name");
        auto num_en = req.url_params.get("num");
        auto min_num_en = req.url_params.get("min");
        int supply_id = 0;
        if (not supply.empty()){
            int supply_id = supply.back().id;
        }
        
        if (!name || !num_en || !min_num_en) {
            return crow::response(404, "Please enter data");
        }
        int num = stoi(num_en);
        int min_num = stoi(min_num_en);
        Supply supply_n(supply_id+1, name, num, min_num);
        supply.emplace_back(supply_n);

        sqlite3* db;
        int rc = sqlite3_open("hospital.db", &db);
        addSupply(db, name, num, min_num);
        sqlite3_close(db);
        return crow::response("Supply add successfully, supply ID:" + to_string(supply_n.id));
    });

    CROW_ROUTE(app, "/supply_up")([&supply, &reminds](const crow::request& req) {
        auto id_en = req.url_params.get("id");
        auto num_en = req.url_params.get("n");
        
        if (!id_en || !num_en) {
            return crow::response(404, "Please enter valid data for id and quantity");
        }
        int id = stoi(id_en);
        int num_up = stoi(num_en);

        for (auto& supplyy : supply) {
            if (supplyy.id == id) {
                supplyy.num = num_up;

                if (supplyy.track()) {
                    std::string remind = "Warning: Item with ID " + std::to_string(id) + " is low on stock!";
                    reminds.push_back(remind);
                }

                sqlite3* db;
                int rc = sqlite3_open("hospital.db", &db);
                SupplyUp(db, id, num_up);
                sqlite3_close(db);
                return crow::response("Supply ID " + std::to_string(id) + "Updated successfully ");
            }
        }
        return crow::response(404, "Supply with ID " + std::to_string(id) + " not found");
    });

    CROW_ROUTE(app, "/remind")([&reminds]() {
        std::string response = "[";
        for (size_t i = 0; i < reminds.size(); ++i) {
            response += "\"" + reminds[i] + "\"";
            if (i != reminds.size() - 1) response += ", ";
        }
        response += "]";
        return crow::response(response);
    });

    // Route: Get all doctors
    CROW_ROUTE(app, "/patients")([&patients]() {
        std::string response = "[";
        for (size_t i = 0; i < patients.size(); ++i) {
            response += patients[i].to_json();
            if (i != patients.size() - 1) response += ", ";
        }
        response += "]";
        return crow::response(response);
    });

    CROW_ROUTE(app, "/patient/")([&patients](const crow::request& req) {
        auto patient_id = req.url_params.get("id");
        auto patient = std::find_if(patients.begin(), patients.end(), [patient_id](const Patient& pat) {
        return pat.id == std::stoi(patient_id);
        });
        if (patient == patients.end()) {
            return crow::response(404, "Patient not found");
        }
        return crow::response(patient->to_json());
    });

    CROW_ROUTE(app, "/doctors")([&doctors]() {
        std::string response = "[";
        for (size_t i = 0; i < doctors.size(); ++i) {
            response += doctors[i].to_json();
            if (i != doctors.size() - 1) response += ", ";
        }
        response += "]";
        return crow::response(response);
    });

    //Route: Patient data entry
    CROW_ROUTE(app, "/patient_add")([&patients](const crow::request& req) {
        auto name = req.url_params.get("name");
        auto age_en = req.url_params.get("age");
        int patient_id = 0;
        if (not patients.empty()){
            patient_id = patients.back().id;
        }
        if (!name || !age_en) {
            return crow::response(404, "Please enter data");
        }
        int age = stoi(age_en);
        Patient patient(patient_id+1, name, age);
        patients.emplace_back(patient);
        return crow::response("Patient add successfully, Patient ID:" + to_string(patient.id+1));
    });

    CROW_ROUTE(app, "/patient_add_mh")([&patients](const crow::request& req){
        auto id = req.url_params.get("id");
        auto doctor = req.url_params.get("doctor");
        auto date = req.url_params.get("date");
        auto detail = req.url_params.get("detail");
        if (!id || !doctor || !date || !detail) {
            return crow::response(404, "Please enter data.");
        }
        int p_id = stoi(id);
        auto p_match = find_if(patients.begin(), patients.end(), [p_id](const Patient& patient) {
            return patient.id == p_id;
        });
        if (p_match == patients.end()) {
            return crow::response(404, "Please enter correct patient ID.");
        }
        tm date_tr = {};
        istringstream(date) >> std::get_time(&date_tr, "%Y-%m-%d %H:%M");

        string doctorn = doctor;
        string detailn = detail;
        MedicalHistory mh(doctorn, date_tr, detailn);
        p_match->medicalhistory_add(mh);
        return crow::response("Enter successfully!");
    });

    //Route: Retireve patient’s medical history
    CROW_ROUTE(app, "/patient_mh")([&patients](const crow::request& req){
        auto id = req.url_params.get("id");
        auto doctor = req.url_params.get("doctor");
        auto date = req.url_params.get("date");
        if (!id) {
            return crow::response(404, "Please enter data.");
        }
        int p_id = stoi(id);
        auto p_match = find_if(patients.begin(), patients.end(), [p_id](const Patient& patient) {
            return patient.id == p_id;
        });
        if (p_match == patients.end()) {
            return crow::response(404, "Please enter correct patient ID.");
        }
        string list = "Here is the compliant medical history(Empty means no records found):";
        auto datas = p_match->medicalhistory_get();
        for (auto data : datas) {
            bool search =true;
            if(doctor&&data.doctor != doctor) {
                search = false;
            }
            if(date) {
                tm date_tr = {};
                istringstream(date) >> std::get_time(&date_tr, "%Y-%m-%d %H:%M");
                if (mktime(&date_tr) != mktime(&data.date)) {
                    search = false;
                }
            }
            if (search) {
                list += data.to_json() + ", ";
            }
        }
        return crow::response(list);
    });

    //Route: All patient's bill(?id=&billid=&amount=&date=&detail=)
    CROW_ROUTE(app, "/patient_add_bill")([&patients](const crow::request& req) {
        auto id = req.url_params.get("id");
        auto billid = req.url_params.get("billid");
        auto amount = req.url_params.get("amount");
        auto date = req.url_params.get("date");
        auto detail = req.url_params.get("detail");

        if (!id || !billid || !amount || !date || !detail) {
            return crow::response(404, "Please enter complete data.");
        }
        int p_id = stoi(id);
        auto p_match = find_if(patients.begin(), patients.end(), [p_id](const Patient& patient) {
            return patient.id == p_id;
        });
        if (p_match == patients.end()) {
            return crow::response(404, "Patient not found.");
        }
        tm date_tr = {};
        istringstream(date) >> std::get_time(&date_tr, "%Y-%m-%d %H:%M");
        Bill bill(billid, amount, date_tr, detail);
        p_match->bill_add(bill);
        return crow::response("Bill added successfully.");
    });

    //Route: Retrieve patient's bills (?id=)
    CROW_ROUTE(app, "/patient_bill")([&patients](const crow::request& req) {
        auto id = req.url_params.get("id");
        if (!id) {
            return crow::response(404, "Please enter the patient ID.");
        }
        int p_id = stoi(id);
        auto p_match = find_if(patients.begin(), patients.end(), [p_id](const Patient& patient) {
            return patient.id == p_id;
        });
        if (p_match == patients.end()) {
            return crow::response(404, "Patient not found.");
        }
        string list = "[";
        auto bills = p_match->bill_get();
        for (size_t i = 0; i < bills.size(); ++i) {
            list += bills[i].to_json();
            if (i < bills.size() - 1) {
                list += ", ";
            }
        }
        list += "]";
        return crow::response(list);
    });

    //Route: Submit insurance (?id=&billid=)
    CROW_ROUTE(app, "/patient_ins")([&patients](const crow::request& req) {
        auto id = req.url_params.get("id");
        auto billid = req.url_params.get("billid");
        if (!id || !billid) {
            return crow::response(404, "Please enter the ID.");
        }
        int p_id = stoi(id);
        auto p_match = find_if(patients.begin(), patients.end(), [p_id](Patient& patient) {
            return patient.id == p_id;
        });
        if (p_match == patients.end()) {
            return crow::response(404, "Patient not found.");
        }
        auto billlist = p_match->bill_get();
        auto b_match = find_if(billlist.begin(), billlist.end(), [billid](Bill& bill) {
            return bill.billid == billid;
        });

        if (b_match == billlist.end()) {
            return crow::response(404, "Bill not found.");
        }
        b_match->insurance_submit();

        string list = "[Submit successful: ";
        for (size_t i = 0; i < billlist.size(); ++i) {
            list += billlist[i].to_json();
            if (i < billlist.size() - 1) {
                list += ", ";
            }
        }
        list += "]";
        return crow::response(list);
    });

    CROW_ROUTE(app, "/make_appointment/")([&doctors, &patients](const crow::request& req) {
        auto patient_id = req.url_params.get("patient_id");
        auto doctors_id = req.url_params.get("doctors_id");
        auto time = req.url_params.get("time");
        try{
            auto doctor = std::find_if(doctors.begin(), doctors.end(), [doctors_id](const Doctor& doc) {
            return doc.id == std::stoi(doctors_id);
            });

            auto patient = std::find_if(patients.begin(), patients.end(), [patient_id](const Patient& pat) {
                return pat.id == std::stoi(patient_id);
            });
            
            if (doctor != doctors.end()) {
                if (doctor->appointments[time] == "available"){
                    doctor->appointments[time] = "Booked by " + patient->name;
                    return crow::response("Appointment booked");
                }
                else{
                    return crow::response("Please select another appointment");
                }
            }
            else {
                return crow::response(404, "Doctor not found");
            }
            }
            catch (...){
                    return crow::response(404, "An error has occured, check the inputted data");
        }
    });


CROW_ROUTE(app, "/add_prescription/")([&doctors, &patients](const crow::request& req) {
    auto patient_id = req.url_params.get("patient_id");
    auto doctors_id = req.url_params.get("doctors_id");
    auto prescription_name = req.url_params.get("prescription_name");
    auto prescription_amount = req.url_params.get("amount");
    auto prescription_duration = req.url_params.get("duration");

    if (!patient_id || !doctors_id || !prescription_name || !prescription_amount || !prescription_duration) {
        return crow::response("Please enter the required data");
    }

    try {
        auto doctor = std::find_if(doctors.begin(), doctors.end(), [doctors_id](const Doctor& doc) {
            return doc.id == std::stoi(doctors_id);
        });
        if (doctor == doctors.end()) {
            return crow::response(404, "Doctor not found");
        }
        auto patient = std::find_if(patients.begin(), patients.end(), [patient_id](const Patient& pat) {
            return pat.id == std::stoi(patient_id);
        });
        if (patient == patients.end()) {
            return crow::response(404, "Patient not found");
        }
        
        tm current_time = {};
        time_t now = time(nullptr);
        current_time = *localtime(&now);

        Prescription new_prescription(patient->name, doctor->name, current_time, prescription_name, prescription_amount, prescription_duration);
        if (!patient->medicalhistory.empty()) {
            patient->medicalhistory.back().prescription_add(new_prescription);
        }
        else {
            MedicalHistory mh(doctor->name, current_time, "Prescription added");
            mh.prescription_add(new_prescription);
            patient->medicalhistory_add(mh);
        }
        

        return crow::response("Prescription added successfully");
    } catch (...) {
        return crow::response("An  error occurred");
    }
});
    // Start the server
    app.port(18080).multithreaded().run();
}
