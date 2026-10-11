


typedef struct app_interface
{
	// TODO: Can add any kind of information here that might be relevant for friday
	// even the function pointers like Update could go in here and be filled in by
	// the initial load of the app?
	// For now, internal is managed by the app instead
	void *App;
}
app_interface;