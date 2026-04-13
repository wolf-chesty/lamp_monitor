# lamp_monitor
Lamp monitor is an attempt to implement some advanced features for Yealink SIP phones attached to a Asterisk SIP server. Currently, lamp_monitor is able to update some lamp states and return simple menus (like call parked lists, parked call details) and external phonebooks pulled from Asterisk configuration files (like pjsip_wizard.conf). 

Though the initial implementation was written and tested against Yealink T8x Android based SIP phones some effort has gone into decoupling UI specific code. In theory, this application can be extended with additional deskphone types. 

