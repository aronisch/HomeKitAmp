{
  "targets": [
    {
      "target_name": "HomeKitAmp",
      "sources": [ "HomeKitAmpAddon.cpp" ], 
      "cflags": ["-Wall", "-std=c++11"],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
	  "ldflags": ["-lrf24network", "-lrf24", "-lstdc++'],
      "include_dirs" : [
		"<!(node -e \"require('nan')\")", 
		"<!(node -e \"require('streaming-worker-sdk')\")"
		] 
    }
  ]
}