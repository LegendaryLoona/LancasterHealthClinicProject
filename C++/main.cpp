#include "crow.h"
//#include "crow_all.h"
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <map>
#include"classes.h"
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

    CROW_ROUTE(app, "/doctors")([&doctors]() {
        std::string response = "[";
        for (size_t i = 0; i < doctors.size(); ++i) {
            response += doctors[i].to_json();
            if (i != doctors.size() - 1) response += ", ";
        }
        response += "]";
        return crow::response(response);
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

    // Start the server
    app.port(18080).multithreaded().run();
}
