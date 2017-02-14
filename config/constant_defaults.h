// ============== Compile time options - Set these

/*
Choose kernel. 
Phi1 [1], Phi2 [2] - Default, Phi3 [3], Phi4 [4],  Phi4c [5]
*/
#ifndef HEMOCELL_KERNEL
#define HEMOCELL_KERNEL 2
#endif

/*
Choose material model.
1 - Dao/Suresh model (+Fedosov 2010 improvements)
2 - HO model (Under testing)
*/
#ifndef HEMOCELL_MATERIAL_MODEL
#define HEMOCELL_MATERIAL_MODEL 2
#endif

/*
Choose material integration method.
Euler [1], Adams-Bashforth [2]
*/
#ifndef HEMOCELL_MATERIAL_INTEGRATION
#define HEMOCELL_MATERIAL_INTEGRATION 1
#endif

/*
Choose collision operator for LBM.
[1] - BGK <- use this for dt < 0 settings to have tau = 1 suppress oscillations
[2] - MRT <- use this in every other case
*/
#ifndef HEMOCELL_CFD_DYNAMICS
#define HEMOCELL_CFD_DYNAMICS 1
#endif

//TODO: REMOVE!!!!!!!
#ifndef DESCRIPTOR
#if HEMOCELL_CFD_DYNAMICS == 1
    #define DESCRIPTOR descriptors::ForcedD3Q19Descriptor    // Using this with tau == 1 will increase numeric stability. (Suppresses oscillations).
#elif HEMOCELL_CFD_DYNAMICS == 2
    #define DESCRIPTOR descriptors::ForcedMRTD3Q19Descriptor    //Use this whenever tau != 1
#endif
#endif


/*
Choose bending force implementation. This is a numerical modelling choice.
[1] - Local only bending implementation acting on the two opposing vertices of the surfaces with the bending angle.
[2] - Distributed bending using all four vertices with wieghting.
[3] - Distributed bending using all four vertices.
Note: 	[1] is advised for cases where structural rigidity is needed. 
		[2] is an intermediate modell between [1] and [3] using four vertices, however, weighting them. So far seem to be the best option.
		[3] is useful for having increased numerical stability (req.: dx<= 0.5 um, otherwise oscillates). 
*/
#ifndef HEMOCELL_MEMBRANE_BENDING
#define HEMOCELL_MEMBRANE_BENDING 1
#endif

/*
 * Particle Field Type:
 * Correct options are:
 * DenseParticleField3D [Palabos]
 * LightParticleField3D [Palabos]
 * HemoParticleField3D  [HemoCell]
 */
#ifndef HEMOCELL_PARTICLE_FIELD
#define HEMOCELL_PARTICLE_FIELD HemoParticleField3D<double, DESCRIPTOR>
#endif
