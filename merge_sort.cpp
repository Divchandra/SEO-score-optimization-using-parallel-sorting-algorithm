#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <omp.h>
#include <chrono>
#include <algorithm>

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

// Function to merge two sorted halves of data array
void merge(std::vector<CSVData>& data, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    std::vector<CSVData> L(n1), R(n2);

    for (int i = 0; i < n1; i++)
        L[i] = data[left + i];
    for (int j = 0; j < n2; j++)
        R[j] = data[mid + 1 + j];

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i].optimizationOpportunities <= R[j].optimizationOpportunities) {
            data[k] = L[i];
            i++;
        } else {
            data[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        data[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        data[k] = R[j];
        j++;
        k++;
    }
}

// Function to perform merge sort
void mergeSort(std::vector<CSVData>& data, int left, int right) {
    if (left >= right)
        return;

    int mid = left + (right - left) / 2;
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            mergeSort(data, left, mid);
        }
        #pragma omp section
        {
            mergeSort(data, mid + 1, right);
        }
    }

    merge(data, left, mid, right);
}

int main() {
    std::string filename = "/home/divi/alexa.com_site_info.csv";
    std::vector<CSVData> data = readCSV(filename);

    auto start = std::chrono::high_resolution_clock::now();

    #pragma omp parallel
    {
        #pragma omp single
        {
            mergeSort(data, 0, data.size() - 1);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedSeconds = end - start;

    double sortingRate = static_cast<double>(data.size()) / elapsedSeconds.count();

    // Output the sorted data, SEO scores, sorting rate, and number of threads or cores used
    for (const auto& d : data) {
        std::cout << "SEO Score for " << d.siteLink << ": " << calculateSEOScore(d) << std::endl;
    }
    std::cout << "Sorting rate: " << sortingRate << " elements per second" << std::endl;
    std::cout << "Time taken to sort: " << elapsedSeconds.count() << " seconds" << std::endl;
    std::cout << "Number of threads/cores: " << omp_get_max_threads() << std::endl;

    return 0;
}