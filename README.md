# Advanced Sight
This is an efficient sight system implementation for Unreal Engine with many more possibilities than the default sight system available inside the engine. Advanced Sight will be mostly useful in games that have some sort of stealth mechanics, for example Deathloop, Dishonored, Zelda to name a few.

# Features
* One controller can have as many sight configs as one wish to instead being limited to just one, with each config allowing you to set custom range, FOV and gain multiplier
* Full control over how quick a controller perceives target and how quickly they forget the last known location
* Multithreaded implementation and cache friendly data structures for fast computation
* Each sight config has individual sight gain multiplier meaning that you can have very long range sight config that takes a lot of time for the controller to perceive the target and very short range config that perceive the target almost instanteniously
* Target perception points allow to define exactly which "body parts" should be considered when testing visibility e.g. only head, or head and shoulds, or only chest. You decide, and you can decide per actor bases
* Detailed debug drawing making it easy to see what is the state of the sight for a controller
* Example content as part of the plugin to help you understand and play with the plugin without having to implement it in your own game
* Easily extendable stress test map so you can quickly run the test on your target platform and see if the performance meets your expectation without investing a lot of time and effort

# Show room
### Debug drawings for a unit with a 3 sight configs
![image](https://github.com/rlewicki/AdvancedSight/assets/10658394/74bb95c9-776c-43ae-88e0-7421b6cca7cf)
