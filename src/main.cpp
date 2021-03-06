#include <fstream>
#include <cmath>
#include <uWS/uWS.h>
#include <iostream>
#include <thread>
#include "Eigen-3.3/Eigen/Core"
#include "json.hpp"
#include "PathPlanner.h"

using namespace std;

// for convenience
using json = nlohmann::json;

// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
string hasData(const string &s) {
    auto found_null = s.find("null");
    auto b1 = s.find_first_of('[');
    auto b2 = s.find_first_of('}');
    if (found_null != string::npos) {
        return "";
    } else if (b1 != string::npos && b2 != string::npos) {
        return s.substr(b1, b2 - b1 + 2);
    }
    return "";
}

/***********************
Define a new PathPlanner instance named pathPlanner
*************************/
PathPlanner pathPlanner;

void update_position(double x, double y, double s, double yaw, double speed) {
    //Update pathPlanner with its latest position from simulator
    pathPlanner.update_position(x, y, s, yaw, speed);
}

void update_state(const vector<double> &previous_path_x, const double &end_path_s,
                  const vector<vector<double>> &sensor_fusion) {
    //pathPlanner has 3 states namely Keep Lane (KL), Lane Change Left (LCL) and Lane Change Right(LCR)
    //update its current preferred state based on current road situation
    pathPlanner.update_state(previous_path_x, end_path_s, sensor_fusion);
}

vector<vector<double>> realize_state(const vector<double> &previous_path_x, const vector<double> &previous_path_y,
                                     const vector<double> &map_waypoints_x, const vector<double> &map_waypoints_y,
                                     const vector<double> &map_waypoints_s) {
    //realize the chosen state by executing the state once it is safe to do so.
    return pathPlanner.realize_state(previous_path_x, previous_path_y, map_waypoints_x, map_waypoints_y,
                                     map_waypoints_s);
}


int main() {
    uWS::Hub h;

    // Load up map values for waypoint's x,y,s and d normalized normal vectors
    vector<double> map_waypoints_x;
    vector<double> map_waypoints_y;
    vector<double> map_waypoints_s;
    vector<double> map_waypoints_dx;
    vector<double> map_waypoints_dy;

    // Waypoint map to read from
    string map_file_ = "../data/highway_map.csv";
    // The max s value before wrapping around the track back to 0
    double max_s = 6945.554;

    ifstream in_map_(map_file_.c_str(), ifstream::in);

    string line;
    while (getline(in_map_, line)) {
        istringstream iss(line);
        double x;
        double y;
        double s;
        double d_x;
        double d_y;
        iss >> x;
        iss >> y;
        iss >> s;
        iss >> d_x;
        iss >> d_y;
        map_waypoints_x.push_back(x);
        map_waypoints_y.push_back(y);
        map_waypoints_s.push_back(s);
        map_waypoints_dx.push_back(d_x);
        map_waypoints_dy.push_back(d_y);
    }

    h.onMessage(
            [&map_waypoints_x, &map_waypoints_y, &map_waypoints_s, &map_waypoints_dx, &map_waypoints_dy](
                    uWS::WebSocket<uWS::SERVER> *ws, char *data, size_t length,
                    uWS::OpCode opCode) {
                // "42" at the start of the message means there's a websocket message event.
                // The 4 signifies a websocket message
                // The 2 signifies a websocket event
                //auto sdata = string(data).substr(0, length);
                //cout << sdata << endl;
                if (length > 2 && data[0] == '4' && data[1] == '2') {

                    auto s = hasData(data);

                    if (!s.empty()) {
                        auto j = json::parse(s);

                        string event = j[0].get<string>();

                        if (event == "telemetry") {
                            // j[1] is the data JSON object

                            // Main pathPlanner's localization Data
                            double car_x = j[1]["x"];
                            double car_y = j[1]["y"];
                            double car_s = j[1]["s"];
                            double car_d = j[1]["d"];
                            double car_yaw = j[1]["yaw"];
                            double car_speed = j[1]["speed"];

                            // Previous path data given to the Planner
                            auto previous_path_x = j[1]["previous_path_x"];
                            auto previous_path_y = j[1]["previous_path_y"];
                            // Previous path's end s and d values
                            double end_path_s = j[1]["end_path_s"];
                            double end_path_d = j[1]["end_path_d"];

                            // Sensor Fusion Data, a list of all other cars on the same side of the road.
                            auto sensor_fusion = j[1]["sensor_fusion"];

                            json msgJson;

                            // TO-DONE: define a path made up of (x,y) points that the pathPlanner will visit sequentially every .02 seconds

                            //Update pathPlanner with its latest position from simulator
                            update_position(car_x, car_y, car_s, car_yaw, car_speed);

                            //pathPlanner has 3 states namely Keep Lane (KL), Lane Change Left (LCL) and Lane Change Right(LCR)
                            //update its current preferred state based on current road situation
                            update_state(previous_path_x, end_path_s, sensor_fusion);

                            //realize the chosen state by executing the state once it is safe to do so.
                            vector<vector<double>> path = realize_state(previous_path_x, previous_path_y,
                                                                        map_waypoints_x,
                                                                        map_waypoints_y, map_waypoints_s);

                            msgJson["next_x"] = path[0];
                            msgJson["next_y"] = path[1];

                            auto msg = "42[\"control\"," + msgJson.dump() + "]";

                            //this_thread::sleep_for(chrono::milliseconds(1000));
                            (*ws).send(msg.data(), msg.length(), uWS::OpCode::TEXT);

                        }
                    } else {
                        // Manual driving
                        std::string msg = "42[\"manual\",{}]";
                        (*ws).send(msg.data(), msg.length(), uWS::OpCode::TEXT);
                    }
                }
            });

    // We don't need this since we're not using HTTP but if it's removed the
    // program
    // doesn't compile :-(
    h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data,
                       size_t, size_t) {
        const std::string s = "<h1>Hello world!</h1>";
        if (req.getUrl().valueLength == 1) {
            res->end(s.data(), s.length());
        } else {
            // i guess this should be done more gracefully?
            res->end(nullptr, 0);
        }
    });

    h.onConnection([&h](uWS::WebSocket<uWS::SERVER> *ws, uWS::HttpRequest req) {
        std::cout << "Connected!!!" << std::endl;
    });

    h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> *ws, int code,
                           char *message, size_t length) {
        (*ws).close();
        std::cout << "Disconnected" << std::endl;
    });

    int port = 4567;
    if (h.listen(port)) {
        std::cout << "Listening to port " << port << std::endl;
    } else {
        std::cerr << "Failed to listen to port" << std::endl;
        return -1;
    }
    h.run();
}

