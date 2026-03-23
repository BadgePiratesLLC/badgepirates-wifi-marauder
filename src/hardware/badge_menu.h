#pragma once
// BSidesKC Badge - Badge-specific menu items
// Adds LED brightness, buzzer mute, battery status, and hardware test to Marauder menu

#ifdef HAS_SCREEN

void badgeMenuSetup();  // Call after menu_function_obj.RunSetup()

#endif
