#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <omp.h>

// Struct to hold data from CSV file
struct CSVData {
    std::string siteLink;
    double optimizationOpportunities;
    double keywordGaps;
    double easyToRankKeywords;
    double buyerKeywords;
    double siteRank;
    double dailyTimeOnSite;
    // Add more fields as needed
};

// Function to convert a string to double, with error handling
double safeStod(const std::string& str) {
    try {
        return std::stod(str);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid argument: " << str << std::endl;
    } catch (const std::out_of_range& e) {
        std::cerr << "Out of range: " << str << std::endl;
    }
    return 0.0; // Return a default value or handle the error as needed
}

// Function to read CSV file and extract relevant data
std::vector<CSVData> readCSV(const std::string& filename) {
    std::vector<CSVData> data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file." << std::endl;
        return data;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;

        CSVData rowData;
        int column = 0;
        while (std::getline(iss, token, ',')) {
            switch (column) {
                case 0: rowData.siteLink = token; break;
                case 1: rowData.optimizationOpportunities = safeStod(token); break;
                case 2: rowData.keywordGaps = safeStod(token); break;
                case 3: rowData.easyToRankKeywords = safeStod(token); break;
                case 4: rowData.buyerKeywords = safeStod(token); break;
                case 5: rowData.siteRank = safeStod(token); break;
                case 6: rowData.dailyTimeOnSite = safeStod(token); break;
                // Add more cases for additional columns
            }
            column++;
        }
        data.push_back(rowData);
    }

    file.close();
    return data;
}

// Function to calculate SEO score
double calculateSEOScore(const CSVData& data) {
    double Weight_1 = 0.25;
    double Weight_2 = 0.20;
    double Weight_3 = 0.15;
    double Weight_4 = 0.10;
    double Weight_5 = 0.20;
    double Weight_6 = 0.10;

    double seoScore = ((data.optimizationOpportunities * Weight_1 + data.keywordGaps * Weight_2 +
                        data.easyToRankKeywords * Weight_3 + data.buyerKeywords * Weight_4 +
                        data.siteRank * Weight_5 + data.dailyTimeOnSite * Weight_6) /
                       (Weight_1 + Weight_2 + Weight_3 + Weight_4 + Weight_5 + Weight_6)) * 100;

    return seoScore;
}

// Bitonic merge function
void bitonicMerge(std::vector<CSVData>& data, int start, int length, bool direction) {
    if (length > 1) {
        int k = length / 2;
        for (int i = start; i < start + k; i++) {
            if ((data[i].optimizationOpportunities > data[i + k].optimizationOpportunities) == direction) {
                std::swap(data[i], data[i + k]);
            }
        }
        bitonicMerge(data, start, k, direction);
        bitonicMerge(data, start + k, k, direction);
    }
}

// Bitonic sort function
void bitonicSort(std::vector<CSVData>& data, int start, int length, bool direction) {
    if (length > 1) {
        int k = length / 2;
        // Sort in ascending order
        #pragma omp parallel sections
        {
            #pragma omp section
            bitonicSort(data, start, k, true);

            #pragma omp section
            bitonicSort(data, start + k, k, false);
        }
        bitonicMerge(data, start, length, direction);
    }
}

int main() {
    std::string filename = "/home/divi/alexa.com_site_info.csv";
    std::vector<CSVData> data = readCSV(filename);

    auto start = std::chrono::high_resolution_clock::now();

    // Calculate SEO scores for all data
    std::vector<double> seoScores(data.size());
    #pragma omp parallel for
    for (size_t i = 0; i < data.size(); ++i) {
        seoScores[i] = calculateSEOScore(data[i]);
    }

    // Sort the data using parallel bitonic sort
    int n = data.size();
    for (int k = 2; k <= n; k *= 2) {
        for (int j = k / 2; j > 0; j /= 2) {
            #pragma omp parallel for
            for (int i = 0; i < n; i += k) {
                bitonicSort(data, i, j, true);
                bitonicSort(data, i + j, j, false);
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedSeconds = end - start;

    // Calculate speedup
    auto startSequential = std::chrono::high_resolution_clock::now();
    // Perform sequential sorting here
    std::sort(data.begin(), data.end(), [](const CSVData& a, const CSVData& b) {
        return a.optimizationOpportunities < b.optimizationOpportunities;
    });
    auto endSequential = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedSecondsSequential = endSequential - startSequential;
    double speedup = elapsedSecondsSequential.count() / elapsedSeconds.count();

    // Calculate sorting rate
    double sortingRate = static_cast<double>(data.size()) / elapsedSeconds.count();

    // Output the sorted data, SEO scores, sorting rate, speedup, and time taken to sort
    for (const auto& d : data) {
        std::cout << "SEO Score for " << d.siteLink << ": " << calculateSEOScore(d) << std::endl;
    }
    std::cout << "Sorting rate: " << sortingRate << " elements per second" << std::endl;
    std::cout << "Time taken to sort: " << elapsedSeconds.count() << " seconds" << std::endl;
    std::cout << "Speedup: " << speedup << std::endl;
    std::cout << "Number of threads/cores: " << omp_get_max_threads() << std::endl;

    return 0;
}