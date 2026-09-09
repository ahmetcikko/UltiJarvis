Ulti Jarvis listens for "Hey Jarvis" and acts on what you say next.

The settings window opens automatically when this installer finishes.
Three things to do there before the assistant will work properly.


1. Add a Groq API key

   Ulti Jarvis cannot understand a command without one. The settings
   window offers "Get a Free API Key" if you do not have one yet, and
   "I Already Have a Key" if you do. Only Groq keys are supported.
   Nothing else in the app will work until a key is saved.


2. Choose the right microphone

   Open "Select Microphone" and pick the input you actually speak into.
   The system default is often the wrong one, such as a webcam or a
   monitor. If the wrong device is selected the wake word is never heard.


3. Restart the daemon if the wake word stops responding

   The daemon is the background process that listens for "Hey Jarvis".
   If it stops responding, open "Manage Daemon" and press
   "Restart Daemon". That resolves most cases where speech is no longer
   being picked up.
