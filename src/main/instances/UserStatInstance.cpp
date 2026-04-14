#include "UserStatInstance.h"

#include "services/AuthenticatedUserService.h"
#include "utils/RatingUtils.h"

#include <algorithm>

bool UserStatInstance::refresh(const std::string &chartsRoot) {
    userRecords      = AuthenticatedUserService::loadCurrentUserVerifiedRecords(chartsRoot);
    currentRating    = 0.0;
    currentPotential = 0.0;

    if (userRecords.records.empty()) return false;

    std::vector<double> evaluations;
    evaluations.reserve(userRecords.records.size());
    for (const auto &key: userRecords.order){
        const auto it = userRecords.records.find(key);
        if (it == userRecords.records.end()) continue;
        evaluations.push_back(rating_utils::singleChartEvaluation(it->second.chart.getDifficulty(), it->second.accuracy));
    }

    std::sort(evaluations.begin(), evaluations.end(), std::greater<>());
    // Statistics logic: take top-50 evaluations and compute sum/average.
    const std::size_t b50Count = std::min<std::size_t>(50, evaluations.size());
    for (std::size_t i = 0; i < b50Count; ++i){
        currentRating += evaluations[i];
    }
    if (b50Count > 0){
        currentPotential = currentRating / static_cast<double>(b50Count);
    }
    return true;
}

const ChartRecordCollection &UserStatInstance::records() const {
    return userRecords;
}

double UserStatInstance::rating() const {
    return currentRating;
}

double UserStatInstance::potential() const {
    return currentPotential;
}

