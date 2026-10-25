#include "AccelCalibrationSolver.h"

#include <QObject>

#include <array>
#include <cmath>
#include <optional>

namespace imu {

namespace {

constexpr int parameterCount = 6; // offset x, y, z, scale x, y, z
using ParameterVector = std::array<double, parameterCount>;
using NormalMatrix = std::array<std::array<double, parameterCount>, parameterCount>;

std::optional<ParameterVector> solveLinearSystem(NormalMatrix matrix, ParameterVector rightHandSide)
{
    for (int pivot = 0; pivot < parameterCount; ++pivot) {
        int bestRow = pivot;
        for (int row = pivot + 1; row < parameterCount; ++row) {
            if (std::abs(matrix[row][pivot]) > std::abs(matrix[bestRow][pivot]))
                bestRow = row;
        }
        if (std::abs(matrix[bestRow][pivot]) < 1e-12)
            return std::nullopt;
        std::swap(matrix[pivot], matrix[bestRow]);
        std::swap(rightHandSide[pivot], rightHandSide[bestRow]);

        for (int row = pivot + 1; row < parameterCount; ++row) {
            const double factor = matrix[row][pivot] / matrix[pivot][pivot];
            for (int column = pivot; column < parameterCount; ++column)
                matrix[row][column] -= factor * matrix[pivot][column];
            rightHandSide[row] -= factor * rightHandSide[pivot];
        }
    }

    ParameterVector solution {};
    for (int row = parameterCount - 1; row >= 0; --row) {
        double sum = rightHandSide[row];
        for (int column = row + 1; column < parameterCount; ++column)
            sum -= matrix[row][column] * solution[column];
        solution[row] = sum / matrix[row][row];
    }
    return solution;
}

std::optional<ParameterVector> initialGuessFromOppositePoses(const std::vector<CalibrationPose> &poses, double gravity)
{
    ParameterVector parameters {};
    for (int axis = 0; axis < 3; ++axis) {
        std::optional<double> pointingUp;
        std::optional<double> pointingDown;
        for (const CalibrationPose &pose : poses) {
            if (pose.upAxis[axis] > 0.5f)
                pointingUp = pose.meanAcceleration[axis];
            else if (pose.upAxis[axis] < -0.5f)
                pointingDown = pose.meanAcceleration[axis];
        }
        if (!pointingUp || !pointingDown || *pointingUp - *pointingDown <= 0.0)
            return std::nullopt;

        parameters[axis] = (*pointingUp + *pointingDown) / 2.0;
        parameters[axis + 3] = 2.0 * gravity / (*pointingUp - *pointingDown);
    }
    return parameters;
}

} // namespace

AccelCalibrationResult AccelCalibrationSolver::solve(const std::vector<CalibrationPose> &poses, double gravity)
{
    AccelCalibrationResult result;

    double squaredErrorSum = 0.0;
    for (const CalibrationPose &pose : poses)
        squaredErrorSum += std::pow(pose.meanAcceleration.length() - gravity, 2.0);
    result.rawRmsError = poses.empty() ? 0.0 : std::sqrt(squaredErrorSum / poses.size());

    std::optional<ParameterVector> initialGuess = initialGuessFromOppositePoses(poses, gravity);
    if (!initialGuess) {
        result.failureReason = QObject::tr("Her eksen için zıt yönlü iki duruş gerekli.");
        return result;
    }

    ParameterVector parameters = *initialGuess;
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        NormalMatrix normalMatrix {};
        ParameterVector gradient {};

        for (const CalibrationPose &pose : poses) {
            std::array<double, 3> centered {};
            std::array<double, 3> corrected {};
            double correctedLength = 0.0;
            for (int axis = 0; axis < 3; ++axis) {
                centered[axis] = pose.meanAcceleration[axis] - parameters[axis];
                corrected[axis] = parameters[axis + 3] * centered[axis];
                correctedLength += corrected[axis] * corrected[axis];
            }
            correctedLength = std::sqrt(correctedLength);

            const double residual = correctedLength - gravity;
            ParameterVector jacobianRow {};
            for (int axis = 0; axis < 3; ++axis) {
                jacobianRow[axis] = -parameters[axis + 3] * corrected[axis] / correctedLength;
                jacobianRow[axis + 3] = corrected[axis] * centered[axis] / correctedLength;
            }

            for (int row = 0; row < parameterCount; ++row) {
                gradient[row] -= jacobianRow[row] * residual;
                for (int column = 0; column < parameterCount; ++column)
                    normalMatrix[row][column] += jacobianRow[row] * jacobianRow[column];
            }
        }

        for (int diagonal = 0; diagonal < parameterCount; ++diagonal)
            normalMatrix[diagonal][diagonal] += 1e-9;

        const std::optional<ParameterVector> step = solveLinearSystem(normalMatrix, gradient);
        if (!step)
            break;

        double stepSizeSquared = 0.0;
        for (int index = 0; index < parameterCount; ++index) {
            parameters[index] += (*step)[index];
            stepSizeSquared += (*step)[index] * (*step)[index];
        }
        if (stepSizeSquared < convergenceThreshold)
            break;
    }

    for (int axis = 0; axis < 3; ++axis) {
        const double offset = parameters[axis];
        const double scale = parameters[axis + 3];
        if (std::abs(offset) > maxAcceptedOffset || scale < minAcceptedScale || scale > maxAcceptedScale) {
            result.failureReason = QObject::tr("Hesaplanan değerler beklenen aralığın dışında. "
                                               "Duruşları sabit bir yüzeyde tekrar kaydedin.");
            return result;
        }
        result.calibration.offset[axis] = static_cast<float>(offset);
        result.calibration.scale[axis] = static_cast<float>(scale);
    }

    result.isValid = true;
    return result;
}

} // namespace imu
