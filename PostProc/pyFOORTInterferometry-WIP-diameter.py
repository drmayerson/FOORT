"""!
@file pyFOORTInterferometry-WIP-diameter.py
@date 2025-03-5
@brief Package for converting FOORT output data into visibility amplitudes and analyzing them.
@author Daniel Mayerson
"""

import pyFOORTInterferometry

# --- VISAMP ANALYSIS FUNCTIONS --- #


def VisampModelPeriodicd(
    u: np.ndarray, d: float, UpEnvFunc: Any, LowEnvFunc: Any
) -> np.ndarray:
    """!
    @brief Model for visibility amplitude, given upper and lower envelope functions.
    @param u: The values to evaluate the model at.
    @param d: The projected diameter of the model circlipse.
    @param UpEnvFunc: The upper envelope function.
    @param LowEnvFunc: The lower envelope function.
    @return VisampModel: The model for the visibility amplitude.
    """
    eMax = np.array(UpEnvFunc(u))
    eMin = np.array(LowEnvFunc(u))
    AlphaL = (eMax + eMin) / 2.0
    AlphaR = (eMax - eMin) / 2.0
    Sindu = np.sin(2 * np.pi * d * np.array(u))

    return np.sqrt(
        np.multiply(AlphaL, AlphaL)
        + np.multiply(AlphaR, AlphaR)
        + 2 * np.multiply(AlphaL, np.multiply(AlphaR, Sindu))
    )


def GetLowerEnvFunc(Vals: np.ndarray) -> interp1d:
    """!
    @brief Gets the lower envelope function from the values using cubic interpolation.
    @param Vals: The values to get the lower envelope function from.
    @return InterpFunc: The lower envelope function.
    """
    # get indices of local minima
    Min_Indices = signal.argrelextrema(Vals, np.less)

    # Get the values at the local minima
    MinimaVals = Vals[Min_Indices]

    # Get the indices of the local minima
    MinimaIndices = Min_Indices[0]

    # Create an cubic interpolation function as lower envolope
    InterpFunc = interp1d(MinimaIndices, MinimaVals, kind="cubic")
    return InterpFunc


# Get the upper envelope function from the values
def GetUpperEnvFunc(Vals: np.ndarray) -> interp1d:
    """!
    @brief Gets the upper envelope function from the values using cubic interpolation.
    @param Vals: The values to get the upper envelope function from.
    @return InterpFunc: The upper envelope function.
    """
    # get indices of local maxima
    Max_Indices = signal.argrelextrema(Vals, np.greater)

    # Get the values at the local maxima
    MaximaVals = Vals[Max_Indices]

    # Get the indices of the local maxima
    MaximaIndices = Max_Indices[0]

    # Create an cubic interpolation function as lower envolope
    InterpFunc = interp1d(MaximaIndices, MaximaVals, kind="cubic")
    return InterpFunc


def RMSDVisampd(
    Udata: np.ndarray, Vals: np.ndarray, dModel: float, UpEnvFunc: Any, LowEnvFunc: Any
) -> float:
    """!
    @brief Calculates the root mean square deviation for the model with parameter dModel evaluated on the data (Udata, Vals).
    @param Udata: The values to evaluate the model at.
    @param Vals: The visamp data.
    @param dModel: The projected diameter of the model circlipse.
    @param UpEnvFunc: The upper envelope function.
    @param LowEnvFunc: The lower envelope function.
    @return RMSD: The root mean square deviation.
    """
    ValModel = VisampModelPeriodicd(Udata, dModel, UpEnvFunc, LowEnvFunc)
    return np.sqrt(np.average(np.square(ValModel - Vals))) / np.average(ValModel)


def GetFitCirclipseds(
    URange: np.ndarray,
    AllVals: np.ndarray,
    GuessMin: float,
    GuessMax: float,
    Steps: int = 5000,
) -> tuple[np.ndarray, np.ndarray]:
    """!
    @brief Finds the best fit circlipse d's and RMSDs for a given range of visamp data.
    @details This function takes a range of (u, visamp(u)) values, and returns all local minima values for projected diameter d in circlipse, together with their RMSDs.
    @param URange: The range of values to evaluate the model at.
    @param AllVals: The visamp data.
    @param GuessMin: The minimum guess for the projected diameter of the model circlipse.
    @param GuessMax: The maximum guess for the projected diameter of the model circlipse.
    @param Steps: The number of steps to take in the range (default = 5000).
    @return Bestds: The best fit d's.
    @return BestRMSDs: The RMSDs for the best fit d's.
    """
    # First, get the lower and upper envelope functions
    UpEnv = GetUpperEnvFunc(AllVals)
    LowEnv = GetLowerEnvFunc(AllVals)

    # Calculate RMSDs for all d's in range
    dRange = np.linspace(GuessMin, GuessMax, Steps)
    RMSDRange = np.array(
        [RMSDVisampd(URange, AllVals[URange], d, UpEnv, LowEnv) for d in dRange]
    )

    # Get all local max indices and values
    Best_Indices = signal.argrelextrema(RMSDRange, np.less)
    # Get the RMSDs at the local maxima
    BestRMSDs = RMSDRange[Best_Indices]
    # Get the indices of the local maxima
    Bestds = dRange[Best_Indices[0]]

    return Bestds, BestRMSDs


def GetFitCirclipsedsMultiple(
    URange: np.ndarray,
    Visamps: np.ndarray,
    GuessMin: float,
    GuessMax: float,
    Steps: int = 5000,
    Verbose: bool = True,
) -> tuple[np.ndarray, np.ndarray]:
    """!
    @brief Finds the best fit circlipse d's and RMSDs for multiple visamp data.
    @details This function calls GetFitCirclipseds for each visamp in Visamps.
    @param URange: The range of values to evaluate the model at.
    @param Visamps: The visamp data.
    @param GuessMin: The minimum guess for the projected diameter of the model circlipse.
    @param GuessMax: The maximum guess for the projected diameter of the model circlipse.
    @param Steps: The number of steps to take in the range (default = 5000).
    @param Verbose: Whether to print out progress information (default = True).
    @return ds: The best fit d's.
    @return RMSDs: The RMSDs for the best fit d's.
    """
    # bestds = np.zeros(len(Visamps), dtype = np.array)
    # bestRMSDs = np.zeros(len(Visamps), dtype = np.array)
    # for i, visamp in enumerate(Visamps):
    if Verbose:
        print(
            "Calculating best fit (circlipse) d's and RMSDs for "
            + f"{len(Visamps)}"
            + " visamps..."
        )

    output = [
        GetFitCirclipseds(URange, visamp, GuessMin, GuessMax, Steps)
        for i, visamp in enumerate(Visamps)
    ]

    ds = [output[i][0] for i in range(len(output))]
    RMSDs = [output[i][1] for i in range(len(output))]

    return ds, RMSDs


def GetAllPhidCollections(Allds, AllRMSDs, Verbose=True):
    NrPhis = len(Allds)
    collections = np.zeros((len(Allds[0]), len(Allds)))
    RMSDcollections = np.zeros(len(Allds[0]))

    for i, collect in enumerate(collections):
        if Verbose:
            print(
                "Calculating matching d's collection "
                + f"{i+1}"
                + " of "
                + f"{len(collections)}"
                + "..."
            )

        # fix first d
        collect[0] = Allds[0][i]
        RMSDcollections[i] = AllRMSDs[0][i]

        for j in range(1, NrPhis):
            # find d that is closest to previous d
            alldistances = [
                np.abs(collect[j - 1] - Allds[j][k]) for k in range(len(Allds[j]))
            ]
            maxindex = np.array(alldistances).argmax()
            collect[j] = Allds[j][maxindex]
            RMSDcollections[i] = RMSDcollections[i] + AllRMSDs[j][maxindex]

    return collections, RMSDcollections


# (phi, visamp) pairs, urange
# call GetFitCirlipseds on each visamp -> collection of (phi, d(phi), RMSD(d(phi)))
# done with phi_n: select from phi_{n+1} the option of d(phi_n+1) that is closest to d(phi_n)
# compare total probabilities of different collections


# still todo:
# - apodization??
# - alter GetBestFitCirclipse to return multiple values and their RMSD