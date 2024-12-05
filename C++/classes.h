#include "crow.h"
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <map>


class Patient {
public:
    int id;                    
    std::string name;          
    int age;
    // Constructor
    Patient(int id, std::string name, int age)
        : id(id), name(name), age(age){}
    // Method to convert Doctor details to JSON-like format
    std::string to_json() const {
        return "{ \"id\": " + std::to_string(id) +
               "\", \"name\": \"" + name +
               "\", \"age\": \"" + std::to_string(age) + "\"}";
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
