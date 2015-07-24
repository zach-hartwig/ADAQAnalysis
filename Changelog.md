# ADAQAnalysis Version Change Log

name: Changelog.md  
date: 28 Apr 15 (last updated)  
auth: Zach Hartwig  
mail: hartwig@psfc.mit.edu  


## Changes in master HEAD

## Version-1.2 Series

Major new developments in this series include:

 - Ability to create spectra and PSD histograms directly from waveform
   data stored within an ADAQ file. This waveform data is created in
   real time by ADAQAcquisition and stored in the ADAQ file if the
   user has selected to do so. The waveform data is primarily energy
   values (e.g. pulse height and pulse area) and PSD integrals
   (e.g. the tail ('short') and total ('long') waveform integrals.
   This feature enables extremely fast creation of spectra and PSD
   histograms while minimizing the required storage space of ADAQ
   files on disk if the user opts to *not* store the raw waveforms
   along with the analyzed waveform data.

 - New feature separates waveform processing and spectrum creation
   such that (for a given set of waveform settings) the waveforms only
   need to be processed *once* and can then be calibrated, rebinned,
   spectra type, etc, without having to reprocess the waveforms
   additional times for each operation. This is extremely useful for
   large data sets where the process of calibration or rebinning, for
   example, was extremely CPU and time intensive. During processing,
   pulse height and pulse area are both calculated and stored for
   later use, enabling near-instant switching between pulse height and
   pulse area spectra.

### 1.2.1

 - Corrections to Makefile to ensure easier building across Linux platfors

 - Restrict waveform analysis to user-specified region in
   time. Applies for spectrum and PSD histogram creation

 - Completely split waveform processing and PSD histogram creation to
   ensure optimal PSD. Implemented for sequential and parallel
   architectures. Various PSD button behavior erros fixed.

 - Updates and optimization of graphics, graphical default settings,
   GUI buttons for custom axes properties

 - New PSD color palette selection option

 - Calibration vectors are now automatically sorted to that
   calibration points can be added in any order

 - Automatic replotting of waveforms when the user clicks to autorange
   the y axis during waveform plotting
   	 
### 1.2.0

 - Correctly set spectrum TH1F X-axis limits when mininmum bin is
   zero. This fix also suppresses visualization of the under/overflow
   bins, which previously caused undesired display of spectrum.

 - Correctly set spectrum TH1F X-axis title based on spectrum type
   (pulse area/height) and calibration state (keV, MeV)

 - Fixed bug in state of spectrum process/create text buttons when
   loading ASIM files


## Version-1.0 Series

Major new developments in this series include:

### 1.0.1

 - Enabled spectrum background min/max limits in floats instead of
   restricting to integer values

 - Ability to specify the title for PSD histogram plotting corrected

 - Casting enumerators to Int_ts to prevent MPI compiler warnings
