#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  std::ifstream input(argv[1]);
  std::string line;
  std::getline(input, line);
  float accumulated_x = 0.0f, accumulated_y = 0.0f;
  int frame = 0;
  std::cout << "frame,velocity_x,velocity_y,delta_x,delta_y,pixel_x,pixel_y\n";
  while (std::getline(input, line)) {
    if (line.empty()) continue;
    std::stringstream row(line);
    std::string field;
    float values[5]{};
    for (int i = 0; i < 5; ++i) { std::getline(row, field, ','); values[i] = std::stof(field); }
    const float dt = values[0];
    const bool held = values[1] != 0.0f;
    float gyro_x = -values[3];
    float gyro_y = -values[2];
    float magnitude = std::sqrt(gyro_x * gyro_x + gyro_y * gyro_y);

    const float cutoff_speed = 1.0f, cutoff_recovery = 3.0f;
    float ignore_factor = (magnitude - cutoff_speed) / (cutoff_recovery - cutoff_speed);
    if (ignore_factor < 1.0f) {
      if (ignore_factor <= 0.0f) gyro_x = gyro_y = magnitude = 0.0f;
      else { gyro_x *= ignore_factor; gyro_y *= ignore_factor; magnitude *= ignore_factor; }
    }
    if (!held) gyro_x = gyro_y = 0.0f;

    const float min_threshold = 10.0f, max_threshold = 100.0f;
    magnitude -= min_threshold;
    if (magnitude < 0.0f) magnitude = 0.0f;
    float sensitivity = magnitude / (max_threshold - min_threshold);
    if (sensitivity > 1.0f) sensitivity = 1.0f;
    float velocity_x = gyro_x * (0.5f * (1.0f - sensitivity) + 2.0f * sensitivity);
    float velocity_y = gyro_y * (0.75f * (1.0f - sensitivity) + 1.5f * sensitivity);
    float delta_x = velocity_x * 40.0f * dt;
    float delta_y = velocity_y * 40.0f * dt;
    accumulated_x += delta_x;
    accumulated_y += delta_y;
    int pixel_x = static_cast<int>(accumulated_x);
    int pixel_y = static_cast<int>(accumulated_y);
    accumulated_x -= pixel_x;
    accumulated_y -= pixel_y;
    std::cout << frame++ << ',' << std::fixed << std::setprecision(9)
      << velocity_x << ',' << velocity_y << ',' << delta_x << ',' << delta_y << ',' << pixel_x << ',' << pixel_y << '\n';
  }
}
