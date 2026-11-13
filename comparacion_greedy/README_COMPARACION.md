# Comparacion aislada de variantes Greedy

Esta carpeta es una copia independiente del proyecto IA. Los archivos de la raiz del repositorio no se modifican ni se usan como destino de los resultados de esta prueba.

## Variantes incluidas

1. Greedy con backtracking limitado: ordena clientes por menor numero de opciones y mayor demanda; genera inserciones factibles, las ordena por costo y prueba alternativas recursivamente. Limites: 20.000 nodos y 5.000 retrocesos.
2. Greedy sin backtracking: usa el mismo orden de clientes y la misma lista ordenada de inserciones factibles, pero fija la primera alternativa y nunca deshace decisiones anteriores.
3. Greedy secuencial por insercion: en cada paso considera todos los clientes pendientes y todas sus inserciones factibles, y fija la de menor costo incremental. No retrocede.

Las tres variantes usan las estructuras actuales de Instance, Solution, Route y Stop, y la validacion final de evaluateSolution. Las ventanas temporales se validan sobre el inicio del servicio (READY <= serviceStart <= DUE); el tiempo de servicio afecta la llegada a las paradas siguientes.

## Ejecutar

Desde esta carpeta:

    make -f Makefile.variantes clean
    make -f Makefile.variantes test-greedy-variants

Se recorren recursivamente las instancias .txt bajo instances/ y se crea greedy_variants_comparison.csv.

El CSV incluye factibilidad, clientes asignados, nodos, retrocesos, tiempo, funcion objetivo, vehiculos y costo de viaje. La consola informa tambien las intersecciones entre conjuntos de instancias resueltas.

La copia conserva los archivos y Makefile originales del proyecto. La prueba nueva usa test_greedy_variants.cpp y Makefile.variantes; no reutiliza test_greedy.cpp ni test_greedy_sa.cpp.
