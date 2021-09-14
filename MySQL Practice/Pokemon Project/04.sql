#04. Print the names of trainers from Blue city in alphabetical order.
SELECT name
FROM Trainer
WHERE hometown = 'Blue City'
ORDER BY name;
