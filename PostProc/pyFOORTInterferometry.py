"""!
@file pyFOORTInterferometry.py
@date 2025-03-5
@brief Package for converting FOORT output data into visibility amplitudes and analyzing them.
@author Daniel Mayerson
"""

import pyFOORT  # Used to read in FOORT data and convert it to a grid
import numpy as np  # Used for basic math operations
from skimage.transform import (
    radon,
    radon_transform,
)  # Used to calculate radon transform of image
from scipy.fft import fft, fftfreq, fftshift  # Used to calculate FFT of radon transform
import matplotlib.pyplot as plt  # Used to plot images
from scipy.interpolate import interp1d  # Used for interpolating
from scipy import signal  # Used in finding outer envelope functions
from typing import Any  # Used for type hints

# --- BASIC FUNCTIONS TO CREATE VISAMP FROM FOORT DATA AND SAVE/LOAD VISAMPS --- #


def RadonTransform(
    FOORTEmissionGrid: np.ndarray, Angles: Any, Verbose: bool = True
) -> np.ndarray:
    """!
    @brief Calculates the radon transform of a given FOORT emission grid.
    @param FOORTEmissionGrid: The FOORT emission grid to calculate the radon transform of.
    @param Angles: The angles to calculate the radon transform at.
    @param Verbose: Whether to print out progress information (default = True).
    @return radontransf: The radon transform of the FOORT emission grid.
    """
    # returns radon transform
    if Verbose:
        print("Calculating radon transform for " + f"{len(Angles)}" + " angles...")
    radontransf = radon(FOORTEmissionGrid, theta=Angles).transpose()
    if Verbose:
        print("Done calculating radon transform.")
    return radontransf


def RadonToComplexVis(
    FOORTRadon: np.ndarray,
    PaddingFactor: float = 25,
    Verbose: bool = True,
    sample_spacing: float = 1.0,
) -> tuple[np.ndarray, np.ndarray]:
    """!
    @brief Calculates the complex visibility from a given radon transform.
    @param FOORTRadon: The radon transform to calculate the complex visibility from.
    @param PaddingFactor: The factor to pad the radon transform by (default = 25).
    @param Verbose: Whether to print out progress information (default = True).
    @param sample_spacing: The spacing between samples in the radon transform (default = 1.0).
    @return complvis: The complex visibility of the radon transform.
    @return freqs: The frequencies of the FFT of the radon transform.
    @details The complex visibility is calculated by taking the FFT of the radon transform, shifting it to center the zero frequency, and selecting only the positive frequencies.
    @note The padding factor is used to increase the resolution of the FFT.
    @note The sample spacing is used to calculate the frequencies of the FFT. It corresponds to the pixel width in the original image.
    """
    if Verbose:
        print("Calculating FFT...")
    radonff = fft(
        FOORTRadon, PaddingFactor * FOORTRadon[0].shape[0]
    )  # 1D FFT of the projection
    radonshift = fftshift(radonff)  # recenter FFT
    xfourier1 = fftshift(
        fftfreq(PaddingFactor * FOORTRadon[0].shape[0], d=sample_spacing)
    )  # re centered frequencies

    indice1 = np.where((xfourier1 >= 0.0))[
        0
    ]  # select only the positive freqs the FFT is symmetrical anyway
    complvis = radonshift[:, indice1[0] : (indice1[-1] + 1)]
    freqs = xfourier1[indice1[0] : (indice1[-1] + 1)]  # frequencies of the visibilities
    if Verbose:
        print("Done calculating FFT.")
    return complvis, freqs


def ComplexVisToNormVisAmp(ComplexVis: np.ndarray, Verbose: bool = True) -> np.ndarray:
    """!
    @brief Normalizes the complex visibility to get the visibility amplitude.
    @param ComplexVis: The complex visibility to normalize.
    @param Verbose: Whether to print out progress information (default = True).
    @return visamp: The normalized visibility amplitude.
    """
    if Verbose:
        print("Normalizing complex visibility to get visibility amplitude...")

    visamp = np.abs(ComplexVis)

    for i in range(len(visamp)):
        visamp[i] /= visamp[i, 0]

    return visamp


def WriteVisAmpsToFile(
    VisAmps: np.ndarray, FileName: str, FirstLineInfo: str = None
) -> None:
    """!
    @brief Writes the visibility amplitudes to a file.
    @param VisAmps: The visibility amplitudes to write to file.
    @param FileName: The name of the file to write to.
    @param FirstLineInfo: The first line of the file, if applicable (default = None).
    """

    np.savetxt(FileName, VisAmps, header=FirstLineInfo)


def LoadVisAmpsFromFile(
    FileName: str, FirstLineDescription: bool = False
) -> np.ndarray:
    """!
    @brief Loads the visibility amplitudes from a file.
    @param FileName: The name of the file to load from.
    @param FirstLineDescription: Whether the first line of the file is a description (default = False).
    @return VisAmps: The visibility amplitudes loaded from the file.
    """
    if FirstLineDescription:
        tempFile = open(FileName, "r")
        FirstLineInfo = tempFile.readline()
        tempFile.close()
        FirstLineInfo = FirstLineInfo[2:]
        return np.loadtxt(FileName), FirstLineInfo
    else:
        return np.loadtxt(FileName)


def DisplayVisAmp(
    VisAmp: np.ndarray,
    URange: tuple[int] = None,
    ImageTitle: str = None,
    FileOutput: str = None,
    Verbose: bool = True,
) -> None:
    """!
    @brief Displays the visibility amplitude.
    @param VisAmp: The visibility amplitude to display.
    @param URange: The range to display (default = None).
    @param ImageTitle: The title of the image (default = None).
    @param FileOutput: The name of the file to save the image to (default = None).
    @param Verbose: Whether to print out progress information (default = True).
    """
    if Verbose:
        print("Displaying image...")

    # plot the picture
    # leave room for title
    if ImageTitle:
        fig = plt.figure(figsize=(8, 12))
        plt.subplots_adjust(top=0.66)
    else:
        fig = plt.figure(figsize=(8, 8))
    ax = plt.axes()

    if URange:
        plt.semilogy(VisAmp[URange[0] : URange[1]])
    else:
        plt.semilogy(VisAmp)

    if ImageTitle:
        plt.suptitle(ImageTitle, fontsize=10, wrap=True)

    # Save the picture to file if applicable
    if FileOutput:
        plt.savefig(FileOutput, format="pdf")
        if Verbose:
            print("Saved image to file " + FileOutput + ".")

    # Show the plot!
    plt.show()
    if Verbose:
        print("Done displaying image.")


def FOORTToVisAmp(
    FilePrefix: str,
    Angles: Any,
    NrFiles: int = 1,
    FirstLineDescription: bool = True,
    Verbose: bool = True,
    GridFraction: float = 1,
    TruncateRange: tuple[float] = None,
    LimitRange: tuple[float] = (0.0, 100000.0),
    EquatPassesRange: tuple[int] = None,
    EquatPassesSelection: list[int] = None,
    RadonPaddingFactor: float = 25,
    FileOutput: str = None,
    LightRingRadius: float = None,
    AngularSize: float = 50.0
    * 1e-6
    * np.pi
    / (180 * 60 * 60),  # in radians (default = 50 microarcseconds)
) -> tuple[np.ndarray, np.ndarray, str]:
    """!
    @brief Converts FOORT output data to visibility amplitudes.
    @param FilePrefix: The prefix of the FOORT output files.
    @param Angles: The angles to calculate the radon transform at.
    @param NrFiles: The number of files to load (default = 1).
    @param FirstLineDescription: Whether the first line of the file is a description (default = True).
    @param Verbose: Whether to print out progress information (default = True).
    @param GridFraction: The fraction of the grid to use (default = 1).
    @param TruncateRange: The range to truncate the data to (default = None).
    @param LimitRange: The range to limit the data to (default = (0.0, 100000.0)).
    @param EquatPassesRange: The range of equatorial passes to select (default = None).
    @param EquatPassesSelection: The selection of equatorial passes to use (default = None).
    @param RadonPaddingFactor: The factor to pad the radon transform by (default = 25).
    @param FileOutput: The name of the file to save the visibility amplitudes to (default = None).
    @param LightRingRadius: The radius of the light ring, if one is present (default = None). When used, this separets "inner" and "outer" photon rings.
    @param AngularSize: Full angular width of the screen in radians, i.e. the angle between the first and last pixel columns, which sit exactly on the screen edges (default = 50 microarcseconds, converted to radians).
    @return visamps: The visibility amplitudes.
    @return baselines: The baselines of the visibility amplitudes corresponding to the visamps.
    @return FirstLineInfo: The first line of the file, if applicable.
    """
    # If a light ring radius is given, we want to separate inner and outer photon rings. Inner photon rings get a minus sign, outer photon rings get a plus sign.
    if LightRingRadius:
        FOORTData, FirstLineInfo = pyFOORT.ModifiedEquatorialEmission(
            FilePrefix,
            NrFiles=NrFiles,
            FirstLineDescription=FirstLineDescription,
            Verbose=Verbose,
            LightRingRadius=LightRingRadius,
        )
    else:
        FOORTData, FirstLineInfo = pyFOORT.LoadFOORTRawData(
            FilePrefix,
            "EquatorialEmission",
            NrFiles=NrFiles,
            FirstLineDescription=FirstLineDescription,
            Verbose=Verbose,
        )

    # Convert data to grid
    FOORTGrid = pyFOORT.DataToGrid(
        FOORTData,
        TruncateRange=TruncateRange,
        LimitRange=LimitRange,
        Diag2RangeSelect=EquatPassesRange,
        Diag2AdvancedSelect=EquatPassesSelection,
        GridFraction=GridFraction,
        Verbose=Verbose,
    )

    # Radon transform
    rad = RadonTransform(FOORTGrid, Angles, Verbose)
    # complex visibility
    complvis, freqs = RadonToComplexVis(
        rad,
        PaddingFactor=RadonPaddingFactor,
        Verbose=Verbose,
        sample_spacing=AngularSize / (FOORTGrid.shape[0]-1),
    )
    baselines = freqs / 1.0e9  # in Giga lambda
    # normalized visamp
    visamps = ComplexVisToNormVisAmp(complvis, Verbose=Verbose)

    if FileOutput:
        WriteVisAmpsToFile(visamps, FileOutput, FirstLineInfo=FirstLineInfo)

    return visamps, baselines, FirstLineInfo
