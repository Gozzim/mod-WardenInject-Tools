-- This script shows how to 

-- Make a global variable
-- Notice how the variable is only initialized if it doesnt exist yet because
-- the persisting variable can be loaded already
PLAYER_STUFF = PLAYER_STUFF or { var = 0 }

-- Then you store data to it or read data from it
PLAYER_STUFF.var = PLAYER_STUFF.var+1
print("This value should increment on reloads of UI:", PLAYER_STUFF.var)
