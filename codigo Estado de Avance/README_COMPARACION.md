
- **Greedy con backtracking limitado:** ordena clientes por menor numero de opciones y mayor demanda; genera inserciones factibles, las ordena por costo y prueba alternativas recursivamente. Limites: 20.000 nodos y 5.000 retrocesos.
- **Greedy sin backtracking:** utiliza el mismo orden de clientes y las mismas inserciones factibles ordenadas; elige la primera alternativa y no deshace decisiones anteriores.

Ambas variantes usan las estructuras actuales de Instance, Solucion, Route y Stop y la validacion final de evaluarSolucion. Las ventanas temporales se validan sobre el inicio del servicio (READY <= serviceStart <= DUE); el tiempo de servicio afecta las llegadas posteriores.

## Ejecutar

    make clean
    make test-greedy-variants

El programa recorre todas las instancias .txt dentro de instances/ y crea greedy_backtracking_comparison.csv. El resumen muestra soluciones factibles, tiempos, objetivo promedio y cuantas instancias resuelve cada variante.