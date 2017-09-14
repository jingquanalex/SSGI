# Screen space global illumination for interactive mixed reality

This project explores screen space rendering methods to approximate global illumination for mixed reality systems, with the objective of a plausible and seamless representation of virtual objects being in a real environment, and running in real-time. Here, the system will be utilising just the screen space input information to calculate light transports between virtual objects, and between real and virtual objects, with the intention of making them look visually plausible rather than physically accurate. The approach I took limits the wide range of global illumination effects to a relevant few that includes indirect diffuse illuminations, glossy reflections and ambient shadows, using screen space sampling and screen space ray marching techniques. These illumination effects directly support the physically based shading for rough and glossy objects using a microfacet reflectance model. Real world geometry is captured using the Microsoft Kinect sensor, and the environment lighting is approximated from a frame of the video to be used as incident light. My results demonstrate various plastic and metallic virtual objects illuminated under various scene lighting running at 60 frames a second.

## Links

* [Thesis pdf](https://drive.google.com/open?id=0B2macuhZeO18Z1pHME5iSW8wblU)
* [Presentation pdf](https://drive.google.com/open?id=0B2macuhZeO18eVh1eDJOXy03WW8)
