# ADAQAnalysis Version Change Log #  

name: Changelog.md  
date: 28 Apr 15 (last updated)  
auth: Zach Hartwig  
mail: hartwig@psfc.mit.edu  


## Version-1.2 Series

Major new developments in this series include:

 - Ability to create spectra and PSD histograms directly from waveform
   data stored within an ADAQ file. This waveform data is created in
   real time by ADAQAcquisition and stored in the ADAQ file if the
   user has selected to do so. The waveform data is primarily energy
   values (e.g. pulse height and pulse area) and'PSD integrals
   (e.g. the tail (``short'') and total (``long'') waveform integrals.
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

      
## Version-1.0 Series

Major new developments in this series include:

### 1.0.1

 - Enabled spectrum background min/max limits in floats instead of
   restricting to integer values

 - Ability to specify the title for PSD histogram plotting corrected

 - Casting enumerators to Int_ts to prevent MPI compiler warnings
