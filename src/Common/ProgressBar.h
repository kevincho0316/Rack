#pragma once
#include <iostream>
#include <string>

struct ProgressBar {
    int total;
    int current = 0;
    int width    = 40;
    std::string label;

    ProgressBar(int total, std::string label = "", int width = 40)
        : total(total), label(std::move(label)), width(width) { render(); }

    void tick() {
        ++current;
        render();
        if (current >= total) std::cout << "\n";
    }

private:
    void render() {
        int filled = (total > 0) ? (current * width / total) : 0;
        std::string bar(filled, '=');
        if (filled < width) {
            bar += '>';
            bar += std::string(width - filled - 1, ' ');
        }
        int pct = (total > 0) ? (current * 100 / total) : 0;
        std::cout << "\r";
        if (!label.empty()) std::cout << label << " ";
        std::cout << "[" << bar << "] "
                  << current << "/" << total
                  << " (" << pct << "%)" << std::flush;
    }
};
