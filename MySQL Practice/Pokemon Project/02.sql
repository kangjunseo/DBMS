#02. Print the nicknames of the Pokémon that are level 50 or higher among the Pokémon you caught, in alphabetical order.
SELECT nickname
FROM CatchedPokemon
WHERE level>=50
ORDER BY nickname;
