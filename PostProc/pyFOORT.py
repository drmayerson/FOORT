"""!
@file pyFOORT.py
@date 2025-03-5
@brief Package for converting FOORT output data into images.
@author Daniel Mayerson
"""

import numpy as np  # Used for various math operations
import pandas as pd  # Used for loading FOORT files
from scipy.interpolate import (
    griddata,
)  # used to create (interpolated) grid from FOORT data
import matplotlib.pyplot as plt  # Used to plot images
from matplotlib import cm  # Color maps
import matplotlib.colors as colors  # Specific colors
from scipy import ndimage  # Used to map/distort background image

# --- BASIC FUNCTIONS FOR ALL DIAGNOSTICS --- #


def LoadFOORTRawData(
    FilePrefix: str,
    DiagType: str,
    NrFiles: int = 1,
    FirstLineDescription: bool = True,
    Verbose: bool = True,
) -> tuple[pd.DataFrame, str]:
    """!
    @brief Load FOORT raw data from file(s) into pandas dataframe
    @param FilePrefix: Prefix of the FOORT output files
    @param DiagType: Type of diagnostic to load, e.g. "FourColorScreen", "EquatorialPasses", "EquatorialEmission", "GeodesicPosition"
    @param NrFiles: Number of files to load (default 1)
    @param FirstLineDescription: Whether the first line of the file contains information (default True)
    @param Verbose: Whether to print progress information (default True)
    @return data: Raw FOORT data as pandas dataframe
    @return FirstLineInfo: Information contained in first line
    """
    # Output file extension is always ".dat"
    Extension = ".dat"

    # Output files contain first line with information
    if FirstLineDescription:
        # must skip first row when reading in data
        skiprows = 1

        tempFile = open(FilePrefix + "_" + DiagType + Extension, "r")
        FirstLineInfo = tempFile.readline()
        tempFile.close()
    else:
        skiprows = 0
        FirstLineInfo = None

    # Say what we are doing
    if Verbose:
        if NrFiles > 1:
            finalpart = " (and " + f"{NrFiles-1}" + " other files)..."
        else:
            finalpart = "..."
        print(
            "Loading FOORT data from "
            + FilePrefix
            + "_"
            + DiagType
            + Extension
            + finalpart
        )

    # Read in (first) file
    data = pd.read_csv(
        FilePrefix + "_" + DiagType + Extension,
        skiprows=skiprows,
        header=None,
        sep="\s+",
    )
    if len(data.columns) == 3:
        data.columns = ["x", "y", "diag1"]
    elif len(data.columns) == 4:  # equatorial emission
        data.columns = ["x", "y", "diag1", "diag2"]
    elif len(data.columns) == 8:  # geodesic position
        data.columns = ["x", "y", "nrpts=1", "separator=;;", "t", "r", "theta", "phi"]
    else:
        raise (
            "Wrong diagnostic output detected in file "
            + FilePrefix
            + "_"
            + DiagType
            + Extension
        )

    # If there are more than 1 files to input, repeat
    if NrFiles > 1:
        df_list = [data]
        for i in range(2, NrFiles + 1):
            new_data = pd.read_csv(
                FilePrefix + "_" + DiagType + f"_{i}" + Extension,
                skiprows=skiprows,
                header=None,
                sep="\s+",
            )
            if len(new_data.columns) == 3:
                new_data.columns = ["x", "y", "diag1"]
            elif len(new_data.columns) == 4:  # equatorial emission
                new_data.columns = ["x", "y", "diag1", "diag2"]
            elif len(data.columns) == 8:  # geodesic position
                data.columns = [
                    "x",
                    "y",
                    "nrpts=1",
                    "separator=;;",
                    "t",
                    "r",
                    "theta",
                    "phi",
                ]
            else:
                raise (
                    "Wrong diagnostic output detected in file "
                    + FilePrefix
                    + "_"
                    + DiagType
                    + f"_{i}"
                    + Extension
                )
        df_list.append(new_data)
        data = pd.concat(df_list)

    # We are done loading!
    if Verbose:
        print("Done loading FOORT data.")
    return data, FirstLineInfo


def DataToGrid(
    FOORTData: pd.DataFrame,
    GridFraction: float = 1,
    DiagToUse: int = 1,
    TakeAbs: bool = False,
    TruncateRange: tuple[int] = None,
    LimitRange: tuple[float] = None,
    Diag2RangeSelect: tuple[int] = None,
    Verbose: bool = True,
) -> np.ndarray:
    """!
    @brief Convert FOORT raw data to grid
    @param FOORTData: Raw FOORT data as pandas dataframe
    @param GridFraction: Fraction of grid size to use (default 1)
    @param DiagToUse: Which diagnostic to use (default 1)
    @param TakeAbs: Whether to take absolute value of data (default False)
    @param TruncateRange: Range to truncate data to (default None)
    @param LimitRange: Range to limit data to (default None)
    @param Diag2RangeSelect: Range of second diagnostic to select (default None)
    @param Verbose: Whether to print progress information (default True)
    @return reshaped_data: Interpolated grid of specified size
    """
    if Verbose:
        print("Reshaping and interpolating grid...")
    # reshape the data, decimated according to GridFraction
    RawGridSize = int(max(FOORTData["x"])) + 1
    NewGridRuler = np.linspace(0, RawGridSize - 1, int(GridFraction * RawGridSize))

    grid_x, grid_y = np.meshgrid(NewGridRuler, NewGridRuler)
    points = np.transpose(np.array([FOORTData["x"], FOORTData["y"]]))

    # Create raw data array with specified diagnostic
    if DiagToUse == 1:
        RawData = np.array(FOORTData["diag1"])
    elif DiagToUse == 2:
        RawData = np.array(FOORTData["diag2"])
    elif DiagToUse == "pos":
        RawData = np.transpose(
            np.array([FOORTData["r"], FOORTData["theta"], FOORTData["phi"]])
        )

    # Optional grid manipulations according to options in function call
    # take absolute value of data (e.g. for equatorial passes, if want to treat geodesics through the horizon equally, as they otherwise
    # have negative values))
    if TakeAbs:
        RawData = np.abs(RawData)
    # Truncate the allowed range (e.g. for equatorial passes: only allow/show geodesics with a given number of passes. Note that TakeAbs
    # has been taken first!)
    if TruncateRange:
        RawData[RawData > TruncateRange[1]] = 0
        RawData[RawData < TruncateRange[0]] = 0
    # Limit the allowed range (e.g. for emission, if there is a minimum/maximum emission we want to allow)
    if LimitRange:
        RawData[RawData > LimitRange[1]] = LimitRange[1]
        RawData[RawData < LimitRange[0]] = LimitRange[0]

    # Only keep values of the first diagnostic (should be equatorial emission)
    # for pixels where the second diagnostic (=equatorial passes) is in a given range
    # note the abs(.) to take absolute value of the equatorial passes!
    if Diag2RangeSelect and DiagToUse == 1:
        RawDataDiag2 = np.abs(np.array(FOORTData["diag2"]))
        # Zero out all values outside the range, and all set values inside the range to 1
        RawDataDiag2[RawDataDiag2 < Diag2RangeSelect[0]] = 0
        RawDataDiag2[RawDataDiag2 > Diag2RangeSelect[1]] = 0
        RawDataDiag2[RawDataDiag2 != 0] = 1
        # Now we can simply multiply these two arrays to select the wanted pixels of the first diagnostic
        RawData = np.multiply(RawData, RawDataDiag2)

    # Create and return (interpolated) grid of specified size
    reshaped_data = griddata(points, RawData, (grid_x, grid_y), method="nearest")
    if Verbose:
        print("Done creating grid.")
    return reshaped_data


def DisplayImage(
    FOORTGrid: np.ndarray,
    TheColorMap: str,
    ColorMinMax: tuple[float] = None,
    ImageTitle: str = None,
    FileOutput: str = None,
    Verbose: bool = True,
) -> None:
    """!
    @brief Display image from grid
    @param FOORTGrid: Grid data to display as image
    @param TheColorMap: Color map to use
    @param ColorMinMax: Min and max values for color map (default None)
    @param ImageTitle: Title of image (default None)
    @param FileOutput: File to save image to (default None)
    @param Verbose: Whether to print progress information (default True)
    """
    if Verbose:
        print("Displaying image...")

    # plot the picture
    # leave room for title
    if ImageTitle:
        fig = plt.figure(figsize=(8, 10.3))
        plt.subplots_adjust(top=0.777)
    else:
        fig = plt.figure(figsize=(8, 8))
    ax = plt.axes()
    if ColorMinMax:
        ax.imshow(
            FOORTGrid,
            vmin=ColorMinMax[0],
            vmax=ColorMinMax[1],
            origin="lower",
            interpolation="nearest",
            cmap=TheColorMap,
        )
    else:
        ax.imshow(FOORTGrid, origin="lower", interpolation="nearest", cmap=TheColorMap)
    plt.axis("off")
    plt.grid(False)

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


# --- SPECIFIC GRID TO IMAGE AND COMBINATION FILE TO IMAGE FUNCTIONS PER DIAGNOSTIC --- #


def GridToFourColorScreenImage(
    FOORTGrid: np.ndarray,
    ImageTitle: str = None,
    FileOutput: str = None,
    Verbose: bool = True,
    NoHorizon: bool = False,
) -> None:
    """!
    @brief Convert grid to four-color screen image
    @param FOORTGrid: Grid data to display as image
    @param ImageTitle: Title of image (default None)
    @param FileOutput: File to save image to (default None)
    @param Verbose: Whether to print progress information
    """
    # Define our own color map for the four-color screen image
    if NoHorizon:
        FourColorScreenColorMap = colors.ListedColormap(
            ["blue", "yellow", "red", "limegreen"]
        )
    else:
        FourColorScreenColorMap = colors.ListedColormap(
            ["black", "blue", "yellow", "red", "limegreen"]
        )

    DisplayImage(
        FOORTGrid,
        FourColorScreenColorMap,
        ImageTitle=ImageTitle,
        FileOutput=FileOutput,
        Verbose=Verbose,
    )


def FOORTToFourColorScreenImage(
    FilePrefix: str,
    NrFiles: int = 1,
    FirstLineDescription: bool = True,
    DisplayImageTitle: bool = False,
    Verbose: bool = False,
    GridFraction: float = 1,
    FileOutput: str = None,
    NoHorizon: bool = False,
) -> None:
    """!
    @brief Convert FOORT output to four-color screen image
    @param FilePrefix: Prefix of the FOORT output files
    @param NrFiles: Number of files to load (default 1)
    @param FirstLineDescription: Whether the first line of the file contains information (default True)
    @param DisplayImageTitle: Whether to display the image title (default False)
    @param Verbose: Whether to print progress information (default False)
    @param GridFraction: Fraction of grid size to use (default 1)
    @param FileOutput: File to save image to (default None)
    """
    # Load in raw FOORT output data
    FOORTData, FirstLineInfo = LoadFOORTRawData(
        FilePrefix,
        "FourColorScreen",
        NrFiles=NrFiles,
        FirstLineDescription=FirstLineDescription,
        Verbose=Verbose,
    )
    if DisplayImageTitle == False:
        FirstLineInfo = None
    # Convert data to grid
    FOORTGrid = DataToGrid(FOORTData, GridFraction=GridFraction, Verbose=Verbose)
    # Display image
    GridToFourColorScreenImage(
        FOORTGrid,
        ImageTitle=FirstLineInfo,
        FileOutput=FileOutput,
        Verbose=Verbose,
        NoHorizon=NoHorizon,
    )


def GridToEquatorialPassesImage(
    FOORTGrid: np.ndarray,
    ImageTitle: str = None,
    FileOutput: str = None,
    Verbose: bool = True,
) -> None:
    """!
    @brief Convert grid to equatorial passes image
    @param FOORTGrid: Grid data to display as image
    @param ImageTitle: Title of image (default None)
    @param FileOutput: File to save image to (default None)
    @param Verbose: Whether to print progress information (default True)
    """
    # Color map for equatorial passes
    EquatorialPassesColorMap = cm.get_cmap(
        "magma", np.max(FOORTGrid) - np.min(FOORTGrid) + 1
    )

    DisplayImage(
        FOORTGrid,
        EquatorialPassesColorMap,
        ImageTitle=ImageTitle,
        FileOutput=FileOutput,
        Verbose=Verbose,
    )


def FOORTToEquatorialPassesImage(
    FilePrefix: str,
    DiagType: str = "EquatorialPasses",
    NrFiles: int = 1,
    FirstLineDescription: bool = True,
    DisplayImageTitle: bool = False,
    PassesRange: tuple[int] = None,
    PlotOnlyInOutBH: int = None,
    PlotAbsPass: bool = True,
    Verbose: bool = False,
    GridFraction: float = 1,
    FileOutput: str = None,
) -> None:
    """!
    @brief Convert FOORT output to equatorial passes image
    @param FilePrefix: Prefix of the FOORT output files
    @param DiagType: Type of diagnostic to load, e.g. "EquatorialPasses", "EquatorialEmission"
    @param NrFiles: Number of files to load (default 1)
    @param FirstLineDescription: Whether the first line of the file contains information (default True)
    @param DisplayImageTitle: Whether to display the image title (default False)
    @param PassesRange: Range of passes to display (default None)
    @param PlotOnlyInOutBH: Whether to plot only in/out of black hole (default None)
    @param PlotAbsPass: Whether to plot absolute value of passes (default True)
    @param Verbose: Whether to print progress information (default False)
    @param GridFraction: Fraction of grid size to use (default 1)
    @param FileOutput: File to save image to (default None)
    """
    # Load in raw FOORT output data
    FOORTData, FirstLineInfo = LoadFOORTRawData(
        FilePrefix,
        DiagType,
        NrFiles=NrFiles,
        FirstLineDescription=FirstLineDescription,
        Verbose=Verbose,
    )
    if DisplayImageTitle == False:
        FirstLineInfo = None

    # Original data was equatorial passes or equatorial emission (which contains passes as second diagnostic)
    if DiagType == "EquatorialPasses":
        DiagToUse = 1
    else:  # DiagType == "EquatorialEmission":
        DiagToUse = 2

    # Perform any truncation of data necessary
    TruncateRange = None
    if PlotOnlyInOutBH == 0:
        TruncateRange = (-10000, 0)
        PlotAbsPass = False
    elif PlotOnlyInOutBH == 1:
        TruncateRange = (0, 10000)
        PlotAbsPass = False
    if PassesRange:
        TruncateRange = PassesRange

    # Convert data to grid
    FOORTGrid = DataToGrid(
        FOORTData,
        DiagToUse=DiagToUse,
        TakeAbs=PlotAbsPass,
        TruncateRange=TruncateRange,
        GridFraction=GridFraction,
        Verbose=Verbose,
    )

    # Display image
    GridToEquatorialPassesImage(
        FOORTGrid, ImageTitle=FirstLineInfo, FileOutput=FileOutput, Verbose=Verbose
    )


def GridToEquatorialEmissionImage(
    FOORTGrid: np.ndarray,
    ImageTitle: str = None,
    FileOutput: str = None,
    Verbose: bool = True,
) -> None:
    """!
    @brief Convert grid to equatorial emission image
    @param FOORTGrid: Grid data to display as image
    @param ImageTitle: Title of image (default None)
    @param FileOutput: File to save image to (default None)
    @param Verbose: Whether to print progress information (default True)
    """
    max_ring = np.max(FOORTGrid)
    min_ring = np.min(FOORTGrid)

    DisplayImage(
        FOORTGrid,
        "afmhot",
        ColorMinMax=(min_ring, max_ring * 1.2),
        ImageTitle=ImageTitle,
        FileOutput=FileOutput,
        Verbose=Verbose,
    )


def FOORTToEquatorialEmissionImage(
    FilePrefix: str,
    NrFiles: int = 1,
    FirstLineDescription: bool = True,
    DisplayImageTitle: bool = False,
    Verbose: bool = False,
    GridFraction: float = 1,
    TruncateRange: tuple[float] = None,
    LimitRange: tuple[float] = (0.0, 100000.0),
    EquatPassesRange: tuple[int] = None,
    FileOutput: str = None,
) -> None:
    """!
    @brief Convert FOORT output to equatorial emission image
    @param FilePrefix: Prefix of the FOORT output files
    @param NrFiles: Number of files to load (default 1)
    @param FirstLineDescription: Whether the first line of the file contains information (default True)
    @param DisplayImageTitle: Whether to display the image title (default False)
    @param Verbose: Whether to print progress information (default False)
    @param GridFraction: Fraction of grid size to use (default 1)
    @param TruncateRange: Range to truncate data to (default None)
    @param LimitRange: Range to limit data to (default (0., 100000.))
    @param EquatPassesRange: Range of equatorial passes to select (default None)
    @param FileOutput: File to save image to (default None)
    """
    # Load in raw FOORT output data
    FOORTData, FirstLineInfo = LoadFOORTRawData(
        FilePrefix,
        "EquatorialEmission",
        NrFiles=NrFiles,
        FirstLineDescription=FirstLineDescription,
        Verbose=Verbose,
    )
    if DisplayImageTitle == False:
        FirstLineInfo = None

    # Convert data to grid
    FOORTGrid = DataToGrid(
        FOORTData,
        TruncateRange=TruncateRange,
        LimitRange=LimitRange,
        Diag2RangeSelect=EquatPassesRange,
        GridFraction=GridFraction,
        Verbose=Verbose,
    )

    # Display image
    GridToEquatorialEmissionImage(
        FOORTGrid, ImageTitle=FirstLineInfo, FileOutput=FileOutput, Verbose=Verbose
    )


def GridToDistortedImage(
    FOORTGrid: np.ndarray,
    BackgroundImageFileName: str,
    BlackConditionRadius: float = 4.0,
    ImageTitle: str = None,
    FileOutput: str = None,
    Verbose: bool = True,
) -> None:
    """!
    @brief Convert grid to distorted image.
    @details Thanks to Pedro Fernandes for the code that this routine is based on.
    @param FOORTGrid: Grid data to display as image
    @param BackgroundImageFileName: Background image file name
    @param BlackConditionRadius: Radius for black condition (default 4.0)
    @param ImageTitle: Title of image (default None)
    @param FileOutput: File to save image to (default None)
    @param Verbose: Whether to print progress information (default True)
    """
    if Verbose:
        print("Loading background picture, mapping and displaying image...")

    BackgroundData = np.fliplr(np.flipud(plt.imread(BackgroundImageFileName)))
    h, w, p = BackgroundData.shape

    GridPlotData = FOORTGrid[:, :, 1:]  # theta and phi
    GridPlotData[:, :, 0] = np.mod(
        GridPlotData[:, :, 0], np.pi
    )  # theta between 0 and pi
    GridPlotData[:, :, 1] = np.mod(
        GridPlotData[:, :, 1], 2.0 * np.pi
    )  # phi between 0 and 2pi

    # make theta, phi values negative for black pixels (map_coordinates makes these black)
    BlackPixels = FOORTGrid[:, :, 0] > BlackConditionRadius
    GridPlotData[:, :, 0] = np.where(
        BlackPixels, GridPlotData[:, :, 0], GridPlotData[:, :, 1] - 2 * np.pi
    )
    GridPlotData[:, :, 1] = np.where(
        BlackPixels, GridPlotData[:, :, 1], GridPlotData[:, :, 1] - 2 * np.pi
    )

    GridPlotData = np.array(
        [
            GridPlotData[:, :, 0] / np.pi * (h - 1),
            GridPlotData[:, :, 1] / (2.0 * np.pi) * (w - 1),
        ]
    )

    Image = np.ndarray(
        (GridPlotData.shape[1], GridPlotData.shape[2], p),
        dtype=BackgroundData[0, 0, 0].dtype,
    )
    shadowColor = [0.0, 0.0, 0.0, 1.0]  # what is this??
    for ip in range(p):
        c = shadowColor[ip]
        ndimage.map_coordinates(
            BackgroundData[:, :, ip],
            GridPlotData,
            mode="constant",
            cval=c,
            order=0,
            output=Image[:, :, ip],
        )

    # plot the picture
    # leave room for title
    if ImageTitle:
        fig = plt.figure(figsize=(8, 10.3))
        plt.subplots_adjust(top=0.777)
    else:
        fig = plt.figure(figsize=(8, 8))
    ax = plt.axes()
    ax.imshow(Image, origin="lower")
    plt.axis("off")
    plt.grid(False)

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

    # plt.imsave("test.png", Image, origin='upper')


def FOORTToDistortedBackground(
    FilePrefix: str,
    BackgroundFile: str,
    BlackConditionRadius: float = 4.0,
    NrFiles: int = 1,
    FirstLineDescription: bool = True,
    DisplayImageTitle: bool = False,
    Verbose: bool = False,
    GridFraction: float = 1,
    FileOutput: str = None,
) -> None:
    """!
    @brief Convert FOORT output to distorted image
    @param FilePrefix: Prefix of the FOORT output files
    @param BackgroundFile: Background image file name
    @param BlackConditionRadius: Radius for black condition (default 4.0)
    @param NrFiles: Number of files to load (default 1)
    @param FirstLineDescription: Whether the first line of the file contains information (default True)
    @param DisplayImageTitle: Whether to display the image title (default False)
    @param Verbose: Whether to print progress information (default False)
    @param GridFraction: Fraction of grid size to use (default 1)
    @param FileOutput: File to save image to (default None)
    """
    # Load in raw FOORT output data
    FOORTData, FirstLineInfo = LoadFOORTRawData(
        FilePrefix,
        "GeodesicPosition",
        NrFiles=NrFiles,
        FirstLineDescription=FirstLineDescription,
        Verbose=Verbose,
    )
    if DisplayImageTitle == False:
        FirstLineInfo = None
    # Convert data to grid
    FOORTGrid = DataToGrid(
        FOORTData, DiagToUse="pos", GridFraction=GridFraction, Verbose=Verbose
    )
    # Display image
    GridToDistortedImage(
        FOORTGrid,
        BackgroundFile,
        BlackConditionRadius=BlackConditionRadius,
        ImageTitle=FirstLineInfo,
        FileOutput=FileOutput,
        Verbose=Verbose,
    )
