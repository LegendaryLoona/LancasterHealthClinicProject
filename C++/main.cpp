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
using namespace std;

int main() {
    crow::SimpleApp app;

    // Default route
    CROW_ROUTE(app, "/")([]() {
        return "Welcome to the appointment booking system";
    });

    // Sample doctors
    std::vector<Doctor> doctors = {
        Doctor(1, "Dr. Alice Smith", "Cardiology", "9:00", "18:00"),
        Doctor(2, "Dr. Bob Jones", "Neurology", "9:00", "18:00"),
        Doctor(3, "Dr. Carol White", "Pediatrics", "9:00", "18:00")
    };
    std::vector<Patient> patients = {
        Patient(1, "John Hopf", 55),
        Patient(2, "Allison Wern", 44),
        Patient(3, "Sir Munhausen", 33)
    };

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
    int patient_id = 1;
    CROW_ROUTE(app, "/patient_add")([&patients, &patient_id](const crow::request& req) {
        auto name = req.url_params.get("name");
        auto age_en = req.url_params.get("age");
        if (!name || !age_en) {
            return crow::response(404, "Please enter data");
        }
        int age = stoi(age_en);
        Patient patient(patient_id++, name, age);
        patients.emplace_back(patient);
        return crow::response("Patient add successfully, Patient ID:" + to_string(patient.id));
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

    // Route: Get appointments for a single doctor
    CROW_ROUTE(app, "/view_appointments/")([&doctors](const crow::request& req) {
        auto id = req.url_params.get("id");
        for (const auto& doctor : doctors) {
            if (doctor.id == std::stoi(id)) {
                return crow::response(doctor.appointments_to_json());
            }
        }
        return crow::response(404, "Doctor not found");
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

    if (!patient_id || !doctors_id || !prescription_name) {
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
        patient->add_prescription(prescription_name, doctor->name);
        return crow::response("Prescription added successfully");
    } catch (...) {
        return crow::response("An  error occurred");
    }
});
    // Start the server
    app.port(18080).multithreaded().run();
}
