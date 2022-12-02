(* 

diffgeo.m			

A Mathematica package for tensor algebra and calculus, by M. Headrick

February 2013 version 1 (see end of package for changelog)


Before loading this package, the following quantities must be defined:

coord
  a list of the coordinates
metric
  the metric in the form of a matrix
metricsign (optional)
  the sign of the determinant of the metric; if this is not specified then
  Mathematica will attempt to determine it

Furthermore, information about the ranges of coordinates and parameters appearing in the metric may be specified by the variable $Assumptions (this helps Simplify, which processes the output of the functions defined in this package, to do a better job). By default, all coordinates are assumed real.

Example:

coord = { r, theta, phi };
$Assumptions = And[ r>0, theta>0, theta<Pi, Element[phi,Reals] ];
metric = DiagonalMatrix[ { 1, r^2, r^2 Sin[theta]^2 } ];

When the package is loaded, the following tensors and other quantities are calculated by Mathematica (some immediately, others only when they are called):

*)

dimen::usage =		"dimen is the dimensionality of the space (number of coordinates)"
inverse::usage =	"Inverse metric."
Christoffel::usage =	"Christoffel symbol, with the upper index first."
rg::usage =		"Square root of the absolute value of the determinant of the metric."
LeviCivita::usage =	"Levi-Civita tensor, with all indices down; equivalent to volumeForm, but as a tensor (array)."
Riemann::usage =	"Riemann tensor, with first three indices down and fourth index up."
RicciTensor::usage =	"Ricci tensor, with both indices down."
RicciScalar::usage =	"Ricci scalar."
Einstein::usage =	"Einstein tensor, with both indices down."
Weyl::usage =		"Weyl tensor, with all indices down."
Cotton::usage =		"Cotton tensor, with both indices down (defined only in 3 dimensions)."
volumeForm::usage =	"Metric volume form; equivalent to LeviCivita, but as a \[DoubleStruckD] expression."

(*

For the curvature tensors we follow the conventions of Wald.

Index values for tensors can be specified either by number or by the name of the coordinate, as in:

Riemann[[r,theta,r,theta]]

This applies not just to the tensors listed above but to any tensor, and can be used when setting the value of a component of a tensor.

The following functions are also defined:

*)

coordQ::usage =
"coordQ[ expr ] returns True if expr is a coordinate or a list of coordinates, False otherwise."

FullTensorQ::usage =
"FullTensorQ[ expr ] returns True if expr is a full array with dimensionality at all levels equal to the number of coordinates, and False otherwise."

scalarQ::usage =
"scalarQ[ expr ] returns False if expr is or contains a list, a SparseArray object, or a \[DoubleStruckD] object, and returns True otherwise."

rank::usage =
"For a tensor (array), rank[ expr ] returns its rank (number of indices). For a form, it returns the rank if expr has a unique one, and a list of ranks if expr is a sum of forms of forms with different ranks."

display::usage =
"display[ tensor ] displays a list of the distinct non-zero components of tensor, together with the positions in the tensor at which they occur."

transpose::usage =
"transpose[ tensor, {index1,index2,...} ] puts index1 of tensor into first position, index2 into second position, etc.; unlisted indices are left in their current order following the listed ones."

swapIndices::usage =
"swapIndices[ tensor, {index1,index2} ] performs a transpose that swaps index1 and index2, leaving all other indices unaffected." 

(*

The following built-in Mathematica function is also useful for taking transposes:

Transpose[ tensor, {index1,index2,...} ] is the inverse of transpose: puts the first index of tensor into position index1, the second into position index2, etc.

*)

tr::usage =
"tr[ tensor, {index1,index2}, {index3,index4}, . . . (optional) ]: trace over index1 and index2, over index3 and index4, etc. (without using the metric, so one index should be up and the other down). If no index pair is given, it returns the trace over the first two indices."

outer::usage =
"outer[ tensor1, tensor2, ... ] or tensor1 ** tensor2 ** ...  gives the outer product of an arbitrary number of tensors."

NonCommutativeMultiply::usage =
"NonCommutativeMultiply[ tensor1, tensor2, ... ] or tensor1 ** tensor2 ** ...  gives the outer product of an arbitrary number of tensors."

symmetrize::usage =
"symmetrize[ tensor, {index1,index2,...} (optional) ] symmetrizes on the listed indices of tensor; if the list is omitted, symmetrizes on all indices; if multiple lists of indices are given, then it symmetrizes separately on the indices in each list."

antisymmetrize::usage =
"antisymmetrize[ tensor, {index1,index2,...} (optional) ] antisymmetrizes on the listed indices of tensor; if the list is omitted, antisymmetrizes on all indices; if multiple lists of indices are given, then it antisymmetrizes separately on the indices in each list."

symmetricQ::usage =
"symmetricQ[ tensor, {index1,index2,...} (optional) ] tests whether tensor is fully symmetric under interchange of the listed indices; if the list is omitted, tests for symmetry under interchange of all indices."

antisymmetricQ::usage =
"antisymmetricQ[ tensor, {index1,index2,...} (optional) ] tests whether tensor is fully antisymmetric under interchange of the listed indices; if the list is omitted, tests for antisymmetry under interchange of all indices."

zeroQ::usage =
"zeroQ[ tensor ] returns True if all entries of tensor are zero, False otherwise."

zeroTensor::usage =
"zeroTensor[ rank ] returns the zero tensor of the given rank."

contract::usage =
"contract[ tensor, {index1,index2}, {index3,index4}, . . . (optional) ]: contraction of index1 with index2, index3 with index4, etc., using the inverse metric (i.e. all indices to be contracted are assumed to be down). If no index pair is given, it returns the contraction of the first two indices."

raise::usage =
"raise[ tensor, {index1,index2,...} (optional) ] raises index1, index2, etc. using the inverse metric; if the list of indices is omitted then all indices are raised (so they should all be down in the original tensor)."

lower::usage =
"lower[ tensor, {index1,index2,...} (optional) ] lowers index1, index2, etc. using the metric; if the list of indices is omitted then all indices are lowered (so they should all be up in the original tensor)."

norm::usage =
"norm[ tensor ] returns the contraction of tensor with itself. All indices are assumed to be down."

partial::usage =
"partial[ tensor ]: partial derivative of a tensor; the derivative index is the first index of the resulting `tensor'."

divergence::usage =
"divergence[ vector ]: the covariant divergence, where vector is given with an upper index (this also works with any totally antisymmetric tensor with all upper indices)."

scalarLaplacian::usage =
"scalarLaplacian[ scalar ]: Laplacian of the scalar."

covariant::usage =
"covariant[ tensor, indexpositions (optional) ]: covariant derivative of a tensor; the derivative index is the first index of the resulting tensor; the indexpositions argument is a list of the form {up,down,down}; if it is omitted, then all indices are assumed to be down. (The index position none is also allowed, indicating that the corresponding index should be ignored, i.e. no connection should be used.)"

Lie::usage =
"Lie[ vector, tensor, index_positions (optional) ]: Lie derivative of the tensor with respect to the vector; the vector should be a vector, not a co-vector (i.e. it should have an upper index); the index_positions argument gives the positions of all the indices of the tensor; it is a list of the form {up,down,down}; if it is omitted, then all indices are assumed to be down. tensor may also be a form (\[DoubleStruckD] expression), in which case the index_positions argument is omitted."

commutator::usage =
"commutator[ vector1, vector2 ] returns the commutator of the two vector fields."

zeroTensor::usage =
"zeroTensor[ rank ] returns the zero tensor with the given rank."

dd::usage =
"dd[ coordinate1, coordinate2, ... ] or \[DoubleStruckD][ coordinate1, coordinate2, ... ] represents the wedge product of the one-forms associated with the given coordinates. For example, \[DoubleStruckD][ t, r ] represents the two-form dt\[Wedge]dr. The arguments of dd or \[DoubleStruckD] must be coordinates."

\[DoubleStruckD]::usage =
"dd[ coordinate1, coordinate2, ... ] or \[DoubleStruckD][ coordinate1, coordinate2, ... ] represents the wedge product of the one-forms associated with the given coordinates. For example, \[DoubleStruckD][ t, r ] represents the two-form dt\[Wedge]dr. The arguments of dd or \[DoubleStruckD] must be coordinates."

ranks::usage =
"ranks[ form ] returns a list of the distinct ranks of the terms in form."

wedge::usage =
"wedge[ form1, form2, ... ] or form1\[Wedge]form2\[Wedge]...: wedge product of form1, form2, etc. These can either be tensors (arrays) or \[DoubleStruckD] expressions. If tensors and \[DoubleStruckD] expressions are combined then the tensors are first converted to \[DoubleStruckD] expressions before the wedge product is computed."

Wedge::usage =
"Wedge[ form1, form2, ... ] or form1\[Wedge]form2\[Wedge]...: wedge product of form1, form2, etc. These can either be tensors (arrays) or \[DoubleStruckD] expressions. If tensors and \[DoubleStruckD] expressions are combined then the tensors are first converted to \[DoubleStruckD] expressions before the wedge product is computed."

WedgePower::usage =
"WedgePower[ form, n ] returns form wedged with itself n times. form can be either a tensor (array) or a \[DoubleStruckD] expression."

WedgeExp::usage =
"WedgeExp[ form ] returns the wedge exponential of form, which must be a \[DoubleStruckD] expression."

exterior::usage =
"exterior[ form ]: exterior derivative d of form, which can be either a tensor (array) or a \[DoubleStruckD] expression."

HodgeStar::usage =
"HodgeStar[ form ]: Hodge star of form, which can be either a tensor (array) or a \[DoubleStruckD] expression."

formContract::usage =
"formContract[ vector, form ]: contraction of vector with form, which should be a \[DoubleStruckD] expression, returning a \[DoubleStruckD] expression. The contraction is done without the metric (so the first argument should be a vector, not a co-vector)."

FormToTensor::usage =
"FormToTensor[ form ] converts form from a [DoubleStruckD] expression to a tensor (array)."

TensorToForm::usage =
"TensorToForm[ form ] converts form from a totally antisymmetric tensor (array) to a [DoubleStruckD] expression."

hypersurface::usage =
"hypersurface[ coordinate, signature (optional) ]: When this function is called, Mathematica calculates several useful quantities pertaining to the hypersurface on which the specified coordinate is constant: unitnormal (unit normal co-vector, with index down); projector (rank 2 tensor that projects orthogonally onto the hypersurface, with both indices down); extrinsic (extrinsic curvature tensor, with both indices down); extrinsictrace (trace of the extrinsic curvature); induced (induced metric on the hypersurface, as a (D-1) x (D-1) matrix); HScoord (list of coordinates on the hypersurface). The optional argument `signature' is the norm of the unit normal vector, which can be +1 or -1 (null hypersurfaces are not treated); if omitted, it is taken to be +1."

(*

Note that these functions assume all indices on tensors to be tangent- or cotangent-space valued; the program is not set up to deal with indices on auxiliary or internal spaces (e.g. Lie-algebra valued forms).

*)



(* The actual program starts here *)



(* Unprotect variable names, in case the program was previously loaded in the same session *)

Unprotect[
  coordQ,
  dimen,
  NameToNumber,
  FullTensorQ,
  scalarQ,
  rank,
  display,
  padindexlist,
  transpose,
  swapIndices,
  tr,
  outer,
  symmetrize,
  antisymmetrize,
  symmetricQ,
  antisymmetricQ,
  zeroQ,
  inverse,
  contract,
  raise,
  lower,
  norm,
  partial,
  dg,
  Christoffel,
  divergence,
  scalarLaplacian,
  covariant,
  up,
  down,
  none,
  Lie,
  commutator,
  zeroTensor,
  rg,
  LeviCivita,
  Riemann,
  RicciTensor,
  RicciScalar,
  Einstein,
  Weyl,
  Cotton,
  dd,
  \[DoubleStruckD],
  ranks,
  Wedge,
  wedge,
  WedgePower,
  WedgeExp,
  exterior,
  Lietemp,
  volumeForm,
  HodgeStar,
  formContract,
  FormToTensor,
  TensorToForm,
  hypersurface,
  unitnormal,
  projector,
  extrinsic,
  extrinsictrace,
  induced
  ]



(* Set the default value of $Assumptions *)

If[ $Assumptions == True, $Assumptions = Element[coord,Reals] ]



(* Allow index values to be specified by coordinate name rather than number *)

coordQ[ {} ] = False

coordQ[ expr_List ] := And @@ (MemberQ[ coord, # ]& /@ expr )

coordQ[ expr_] := MemberQ[ coord, expr ]

dimen = Length[ coord ]

NameToNumber = Thread[ coord -> Range[dimen] ]

Unprotect[ Part ];

Part /: tensor_[[ indices1___, index_?coordQ, indices2___ ]] :=
  tensor[[ indices1, index /. NameToNumber, indices2 ]] /;
    ( Dimensions[tensor][[ Length[{indices1}] + 1 ]] == dimen )

( tensor_[[ indices1___, index_?coordQ, indices2___ ]] = expr_ ) ^:=
  ( tensor[[ indices1, index /. NameToNumber, indices2 ]] = expr ) /;
    ( Dimensions[tensor][[ Length[{indices1}] + 1 ]] == dimen )

( tensor_[[ indices1___, index_?coordQ, indices2___ ]] := expr_ ) ^:=
  ( tensor[[ indices1, index /. NameToNumber, indices2 ]] := expr ) /;
    ( Dimensions[tensor][[ Length[{indices1}] + 1 ]] == dimen )

Protect[ Part ];

Unprotect[ Extract ]

Extract[ tensor_?ArrayQ, {indices1___, index_?coordQ, indices2___}, h___ ] :=
  Extract[ tensor, {indices1, index /. NameToNumber, indices2}, h ] /;
    ( Dimensions[tensor][[ Length[{indices1}] + 1 ]] == dimen )

Protect[ Extract ]



(* Tensor size and shape testing *)

FullTensorQ[ expr_ ] := FullTensorQ[ expr, dimen ]

FullTensorQ[ expr_, n_Integer ] := ArrayQ[expr] && ( Union[Dimensions[expr]] == {n} )

scalarQ[ expr_ ] := FreeQ[ expr, SparseArray | List | dd ]

rank[ scalar_?scalarQ ] = 0

rank[ tensor_?FullTensorQ ] := ArrayDepth[ tensor ]



(* display *)

display[ tensor_?FullTensorQ ] := display[ tensor, coord ]

display[ tensor_?ArrayQ, indexVals_List ] /;
  (Union[Dimensions[tensor]]=={Length[indexVals]}) :=
  Grid[
    With[
      { tensorAR = ArrayRules[tensor] },
      tensorElements = tensorAR[[All,2]];
      tensorPositions = tensorAR[[All,1]];
      ( {
        Column[
          (indexVals[[#]]&) /@
            tensorPositions[[ Position[ tensorElements, #, 1 ][[All,1]] ]]
          ],
        #
        } & ) /@
        DeleteCases[ Union[ tensorElements ], 0 ]
      ],
    Frame -> { False, All }
    ]



(* Basic tensor algebra *)

padindexlist[ indic_List ] := Join[ indic, Complement[ Range[Max[indic]], indic ] ]

transpose[ tensor_?ArrayQ, indic_List ] :=
  Transpose[ tensor, Ordering[padindexlist[indic]] ]

swapIndices[ tensor_?ArrayQ, {index1_Integer,index2_Integer} ] /; (index1 =!= index2) := 
  With[
    { minindex = Min[index1,index2], maxindex = Max[index1,index2] },
    Transpose[
      tensor, 
      Join[ Range[minindex-1], {maxindex}, Range[minindex+1,maxindex-1], {minindex} ]
      ]
    ]

tr[ tensor_?ArrayQ ] := Tr[tensor, Plus, 2] // Simplify

tr[ tensor_?ArrayQ, indic:{_,_}.. ] :=
  Nest[ tr, transpose[tensor,Join[indic]], Length[{indic}] ]

Unprotect[ NonCommutativeMultiply ];

(scalar_?scalarQ) ** a_ := scalar a // Simplify

a_ ** (scalar_?scalarQ) := scalar a // Simplify

NonCommutativeMultiply[ tensors__?ArrayQ ] := Outer[ Times, tensors ] // Simplify

Protect[ NonCommutativeMultiply ]

outer = NonCommutativeMultiply

symmetrize[ scalar_?scalarQ ] := scalar

symmetrize[ tensor_?ArrayQ ] := symmetrize[ tensor, Range[ ArrayDepth[tensor] ] ]

symmetrize[ tensor_?ArrayQ, indi_List ] := With[
  { temptensor = transpose[tensor,indi],
    numb = Length[indi]
  },
  Transpose[
    Mean[ Map[ Transpose[temptensor,#]&, Permutations[Range[numb]] ] ],
    padindexlist[indi]
    ] // Simplify
  ]

symmetrize[ tensor_?ArrayQ, indi1_List, indi2__List ] :=
  symmetrize[ symmetrize[tensor, indi1], indi2 ]

antisymmetrize[ scalar_?scalarQ ] := scalar

antisymmetrize[ tensor_?ArrayQ ] :=
  antisymmetrize[ tensor, Range[ ArrayDepth[tensor] ] ]

antisymmetrize[ tensor_?ArrayQ, indi_List ] := With[
  { temptensor = transpose[tensor,indi],
    numb = Length[indi]
  },
  Transpose[
    Mean[ Map[ Signature[#]Transpose[temptensor,#]&, Permutations[Range[numb]] ] ],
    padindexlist[indi]
    ] //Simplify
  ]

antisymmetrize[ tensor_?ArrayQ, indi1_List, indi2__List ] :=
  antisymmetrize[ antisymmetrize[tensor, indi1], indi2 ]

symmetricQ[ tensor_?ArrayQ, indices_List ] := 
  Equal @@ Append[ (swapIndices[tensor,#]&) /@ Partition[indices,2,1], tensor ] //
    Simplify

symmetricQ[ tensor_?ArrayQ ] := symmetricQ[ tensor, Range[rank[tensor]] ]

antisymmetricQ[ tensor_?ArrayQ, indices_List ] := 
  Equal @@ Append[ (swapIndices[tensor,#]&) /@ Partition[indices,2,1], -tensor ] //
    Simplify

antisymmetricQ[ tensor_?ArrayQ ] := antisymmetricQ[ tensor, Range[rank[tensor]] ]

zeroQ[ tensor_ ]:= (tensor === 0 tensor)



(* Making sure the metric is kosher *)

If[
  !(SymmetricMatrixQ[metric] && (Length[metric] == dimen)), 
  Print[
    "Warning: the metric given is not a symmetric matrix with the correct dimensions!"
    ];
  Abort[]
  ]



(* Contracting, raising, and lowering indices *)

inverse = Inverse[ metric ] // FullSimplify

contract[ tensor_?ArrayQ ] := tr[ inverse.tensor ]

contract[ tensor_?ArrayQ, indic:{_,_}.. ] := 
  Nest[ tr[inverse.#]&, transpose[tensor,Join[indic]], Length[{indic}] ]

raise[ tensor_?ArrayQ ] := raise[ tensor, Range[ArrayDepth[tensor]] ]

raise[ tensor_?ArrayQ, indic_List ] :=
  Fold[
    Transpose[
      Inner[ Times, #1, inverse, Plus, #2 ],
      Join[ Range[#2-1], Range[#2+1,ArrayDepth[tensor]], {#2} ]
      ] &,
    tensor,
    indic
    ] // Simplify

lower[ tensor_?ArrayQ ] := lower[ tensor, Range[ArrayDepth[tensor]] ]

lower[ tensor_?ArrayQ, indic_List ] :=
  Fold[
    Transpose[
      Inner[ Times, #1, metric, Plus, #2 ],
      Join[ Range[#2-1], Range[#2+1,ArrayDepth[tensor]], {#2} ]
      ] &,
    tensor,
    indic
    ] // Simplify

norm[ tensor_?FullTensorQ ] := With[
  { trank = rank[tensor] },
  contract[
    tensor**tensor,
    Sequence @@ Table[ {i,trank+i}, {i,trank} ]
    ]
  ]



(* Partial and covariant derivatives *)

partial[ tensor_ ] := Map[ D[tensor,#]&, coord ] //Simplify

dg = partial[ metric ]

Christoffel = inverse.(Transpose[dg,{2,1,3}]+Transpose[dg,{3,2,1}]-dg) / 2 //Simplify

Chrfel = tr[Christoffel]

divergence[ vector_?ArrayQ ] := Inner[D,vector,coord,Plus,1] + Chrfel.vector //Simplify

scalarLaplacian[ scalar_?scalarQ ] := divergence[inverse.partial[scalar]]

covariant[ scalar_?scalarQ ] := partial[scalar]

covariant[ tensor_?FullTensorQ ] :=
  covariant[ tensor, ConstantArray[ down, rank[tensor] ] ]

covariant[ tensor_, indexp:{(up|down|none)...} ] := With[
  { trank = Length[indexp],
    Christoffelt = Transpose[ Christoffel, {3,2,1} ]
    },
  partial[tensor] +
    Sum[
      Which[
        indexp[[index]] === down,
        - Transpose[
          Inner[ Times, tensor, Christoffel, Plus, index ],
          Join[ Range[2,index], Range[index+2,trank+1], {1,index+1} ]
          ],
        indexp[[index]] === up,
        Transpose[
          Inner[ Times, tensor, Christoffelt, Plus, index ],
          Join[ Range[2,index], Range[index+2,trank+1], {1,index+1} ]
          ],
        indexp[[index]] === none,
        0
        ],
      {index,trank}
      ]
    ] // Simplify

Lie[ vector_?VectorQ, scalar_?scalarQ ] := vector . partial[scalar] //Simplify

Lie[ vector_?VectorQ, tensor_?ArrayQ ] :=
  Lie[ vector, tensor, ConstantArray[ down, rank[tensor] ] ]

Lie[ vector_?VectorQ, tensor_?ArrayQ, indexp:{(up|down)...} ] := With[
  { trank = Length[indexp],
    dvector1 = partial[vector],
    dvector2 = Transpose[partial[vector]]
    },
  vector.partial[tensor] +
    Sum[
      Which[
        indexp[[index]] === down,
        Transpose[
          Inner[ Times, tensor, dvector2, Plus, index ],
          Join[ Range[1,index-1], Range[index+1,trank], {index} ]
          ],
        indexp[[index]] === up,
        - Transpose[
          Inner[ Times, tensor, dvector1, Plus, index ],
          Join[ Range[1,index-1], Range[index+1,trank], {index} ]
          ]
        ],
      {index,trank}
      ]
    ] // Simplify

commutator[ vector1_?VectorQ, vector2_?VectorQ ] :=
  vector1 . partial[vector2] - vector2 . partial[vector1] // Simplify



(* More predefined scalars and tensors *)

zeroTensor[ trank_Integer ] := ConstantArray[ 0, ConstantArray[ dimen, trank ] ]

rg := (
  Unprotect[rg];
  rg = (
    detmet = Det[metric] //Simplify;
    If[ ValueQ[metricsign]==False, metricsign = FullSimplify[Sign[detmet]] ];
    Sqrt[ metricsign detmet ] //Simplify
    );
  Protect[rg];
  rg
  )

LeviCivita := (
  Unprotect[LeviCivita];
  LeviCivita = rg Normal[ LeviCivitaTensor[ dimen ] ];
  Protect[LeviCivita];
  LeviCivita
  )

Riemann := (
  Unprotect[Riemann];
  Riemann = With[
    {Christoffelt = Transpose[Christoffel,{3,1,2}]},
    2 antisymmetrize[
      Transpose[Christoffelt.Christoffelt,{1,3,2}] - partial[Christoffelt],
      {1,2}
      ]
    ];
  Protect[ Riemann ];
  Riemann
  )

RicciTensor := (
  Unprotect[RicciTensor];
  RicciTensor = (
    tr[ partial[Christoffel] ]
    - partial[Chrfel] 
    + Chrfel . Christoffel
    - tr[ Transpose[Christoffel.Christoffel,{1,3,2}] ]
    ) //Simplify;
  Protect[ RicciTensor ];
  RicciTensor
  )

RicciScalar := (
  Unprotect[RicciScalar];
  RicciScalar = Tr[RicciTensor.inverse] // Simplify;
  Protect[ RicciScalar ];
  RicciScalar
  )

Einstein := (
  Unprotect[Einstein];
  Einstein = RicciTensor - RicciScalar metric / 2 // Simplify;
  Protect[ Einstein ];
  Einstein
  )

Weyl := (
  Unprotect[Weyl];
  Weyl = (
    lower[ Riemann, {4} ] +
    antisymmetrize[
      Transpose[ metric ** (RicciScalar metric/(dimen-1) - 2 RicciTensor), {1,3,4,2} ],
      {1,2}, {3,4}
      ] 2 / (dimen-2)
    ) // Simplify;
  Protect[Weyl];
  Weyl
  )

If[
  dimen == 3,
  Cotton := (
    Unprotect[Cotton];
    Cotton =
      contract[
        LeviCivita **
          antisymmetrize[ covariant[RicciTensor-RicciScalar metric/4], {1,2} ],
        {1,4}, {2,5}
        ] // Simplify
    )
  ]


(* Forms *)

dd = \[DoubleStruckD]

dd[ ] = 1

dd[ coords:(Alternatives@@coord).. ] := With[
  { coordnums = {coords} /. NameToNumber },
  Signature[ coordnums ] dd @@ coord[[ Union[ coordnums ] ]] /;
  coordnums =!= Union[ coordnums ]
  ]

Derivative[__][ dd ] = 0 &

Unprotect[ Coefficient ]

Coefficient[ expr_, -form_dd ] := -Coefficient[ expr, form ]

Protect[ Coefficient ]

ranks[ scalar_?scalarQ ] = {0}

ranks[ scalar_?scalarQ expr_ ] := ranks[ expr ]

ranks[ form_dd ] := { Length[ form ] }

ranks[ expr1_ + expr2_ ] := Union[ ranks[ expr1 ], ranks[ expr2 ] ]

rank[ expr_ ] := ranks[ expr ] /. {num_} -> num

wedge = Wedge

Wedge[ ] = 1

Wedge[ expr1___, Wedge[expr2__], expr3___ ] := Wedge[ expr1, expr2, expr3 ]

Wedge[ expr_ ] := expr

(* Note: The preceding rules implement associativity of Wedge. Another way to do this would be to give Wedge the attribute Flat. However, in this case it would be impossible to assign Wedge[]=1 and Wedge[expr_]:=expr, as they would lead to infinite loops, so we have chosen instead to implement associativity by hand. (Actually, one can make such assignments, but only if one is sure that every Wedge expression other than those two will be evaluated using previously defined rules into an expression that contains no Wedge.) *)

Wedge[ expr1___, expr2_ + expr3_, expr4___ ] :=
  Wedge[ expr1, expr2, expr4 ] + Wedge[ expr1, expr3, expr4 ]

Wedge[ expr1___, scalar_?scalarQ, expr2___ ] := scalar Wedge[ expr1, expr2 ]

Wedge[ expr1___, scalar_?scalarQ expr2_, expr3___ ] := scalar Wedge[ expr1, expr2, expr3 ]

Wedge[ expr1___, form1_dd, form2_dd, expr2___ ] :=
  Wedge[ expr1, Join[ form1, form2 ], expr2 ]

Wedge[ expr1___, form1_?FullTensorQ, form2:Except[_?ArrayQ], expr2___ ] :=
  Wedge[ expr1, TensorToForm[form1], form2, expr2 ]

Wedge[ expr1___, form1:Except[_?ArrayQ], form2_?FullTensorQ, expr2___ ] :=
  Wedge[ expr1, form1, TensorToForm[form2], expr2 ]

Wedge[ forms__?FullTensorQ ] :=
  ( Multinomial @@ rank /@ {forms} ) antisymmetrize[ NonCommutativeMultiply[forms] ]

WedgePower[ expr_, power_Integer /; power >= 0 ] := Wedge @@ ConstantArray[ expr, power ]

WedgeExp[ expr_ ] := With[
  { scalarpart = Simplify[ expr /. {_dd->0} ] },
  Exp[ scalarpart ] *
  Sum[ 1/n! WedgePower[ Simplify[expr-scalarpart], n ], { n, 0, dimen } ]
  ]

exterior[ expr1_ + expr2_ ] := exterior[ expr1 ] + exterior[ expr2 ]

exterior[ scalar_?scalarQ ] := partial[ scalar ] . ( dd /@ coord )

exterior[ scalar_?scalarQ dd[coords__] ] :=
  Total[ ( D[ scalar, # ] Prepend[ dd[coords], # ] & ) /@ Complement[ coord, {coords} ] ]

exterior[ _dd ] = 0

exterior[ scalar_?scalarQ expr_ ] :=
  Wedge[ exterior[ scalar ], expr ] + scalar exterior[ expr ]

exterior[ form_?FullTensorQ ] := (rank[form]+1) antisymmetrize[ partial[form] ]

Lie[ vector_?VectorQ, expr_:(Not[FreeQ[#,dd]]&)] := 
  Lietemp[ vector, Transpose[partial[vector]], expr ]

Lietemp[ vector_?VectorQ, pv_, expr1_ + expr2_ ] :=
  Lietemp[ vector, pv, expr1 ] + Lietemp[ vector, pv, expr2 ]

Lietemp[ vector_?VectorQ, pv_, scalar_?scalarQ expr_ ] :=
  scalar Lietemp[ vector, pv, expr ] + Lie[ vector, scalar ] expr

Lietemp[ vector_?VectorQ, pv_, form_dd ] :=
  Sum[
    pv[[form[[j]]]] . ( ReplacePart[form,j->#]& /@ coord ),
    { j, Length[form] }
    ]

volumeForm := (
  Unprotect[volumeForm];
  volumeForm = rg dd @@ coord;
  Protect[volumeForm];
  volumeForm
  )

HodgeStar[ scalar_?scalarQ ] := scalar volumeForm

HodgeStar[ expr1_ + expr2_ ] := HodgeStar[ expr1 ] + HodgeStar[ expr2 ]

HodgeStar[ scalar_?scalarQ expr_ ] := scalar HodgeStar[ expr ]

HodgeStar[ dd[coords__] ] := (
  rg Total[
    (
      Signature[ Join[ Complement[coord,#], # ] /. NameToNumber ] *
      Det[ inverse[[ {coords}, Complement[coord,#] ]] ] *
      ( dd @@ # )
      & ) /@
    Subsets[ coord, { dimen - Length[{coords}] } ]
    ]
  ) // Simplify

HodgeStar[ form_?FullTensorQ ] := With[
  { trank = ArrayDepth[form] },    
  contract[
    form ** LeviCivita,
    Sequence @@ Transpose[ {Range[trank],Range[trank+1,2trank]} ]
    ] / trank!
  ]

HodgeStarPolchinski[ form_?FullTensorQ ]:= With[
  { trank = ArrayDepth[form] },    
  contract[
    form ** LeviCivita,
    Sequence @@ Transpose[ {Range[trank],Range[dimen+1,dimen+trank]} ]
    ] / trank!
  ]

formContract[ vector_?VectorQ, expr1_ + expr2_ ] :=
  formContract[ vector, expr1 ] + formContract[ vector, expr2 ]

formContract[ vector_?VectorQ, scalar_?scalarQ expr_ ] :=
  scalar formContract[ vector, expr ]

formContract[ vector_?VectorQ, form_dd ] :=
  Sum[ (-1)^(j+1) vector[[form[[j]]]] Drop[form,{j}], {j,Length[form]} ]

FormToTensor[ expr1_ + expr2_ ] := FormToTensor[ expr1 ] + FormToTensor[ expr2 ]

FormToTensor[ scalar_?scalarQ expr_ ] := scalar FormToTensor[ expr ]

FormToTensor[ scalar_?scalarQ ] := scalar

FormToTensor[ dd[coords__] ] :=
  antisymmetrize[
    Normal[
      SparseArray[
        { ({coords}/.NameToNumber) -> Length[{coords}]! },
        ConstantArray[ dimen, Length[{coords}] ]
        ]
      ]
    ]

TensorToForm[ tensor_?FullTensorQ ] := Total[
  ( Extract[ tensor, # ] ( dd @@ # ) & ) /@ Subsets[ coord, {rank[tensor]} ]
  ]



(* Hypersurfaces *)

hypersurface[ co_?(MemberQ[coord,#]&), signature:(1|-1):1 ] := With[
  { conum =  Position[ coord, co ][[1,1]] },
  Unprotect[
    unitnormal,
    projector,
    extrinsic,
    extrinsictrace,
    induced,
    display,
    HSNameToNumber,
    HScoord,
    HScoordQ,
    Part,
    Extract
    ];
  unitnormal =
    Simplify[(signature inverse[[conum,conum]])^(-1/2)] UnitVector[dimen,conum];
  projector = metric - signature unitnormal ** unitnormal // Simplify;
  extrinsic = (1/2) Lie[ raise[unitnormal], projector ];
  extrinsictrace = contract[ extrinsic ];
  induced = Drop[ metric, {conum}, {conum} ];
  display[ tensor_?ArrayQ ] /; (Union[Dimensions[tensor]]=={dimen-1}) :=
    display[ tensor, HScoord ];
  HScoord = DeleteCases[coord,co];
  HSNameToNumber = Thread[ HScoord -> Range[dimen-1] ];
  HScoordQ[ {} ] = False;
  HScoordQ[ expr_List ] := And @@ (MemberQ[ HScoord, # ]& /@ expr );
  HScoordQ[ expr_] := MemberQ[ HScoord, expr ];
  Part /: tensor_[[ indices1___, index_?HScoordQ, indices2___ ]] :=
    tensor[[ indices1, index /. HSNameToNumber, indices2 ]] /;
      ( Dimensions[tensor][[ Length[{indices1}] + 1 ]] == dimen-1 );
  Extract[ tensor_?ArrayQ, {indices1___, index_?HScoordQ, indices2___}, h___ ] :=
    Extract[ tensor, {indices1, index /. HSNameToNumber, indices2}, h ] /;
      ( Dimensions[tensor][[ Length[{indices1}] + 1 ]] == dimen-1 );
  Protect[
    unitnormal,
    projector,
    extrinsic,
    extrinsictrace,
    induced,
    display,
    HSNameToNumber,
    HScoord,
    HScoordQ,
    Part,
    Extract
    ];
  Unprotect[co];
  ]


(* Protection *)

Protect[
  coordQ,
  dimen,
  NameToNumber,
  FullTensorQ,
  scalarQ,
  rank,
  display,
  padindexlist,
  transpose,
  swapIndices,
  tr,
  outer,
  symmetrize,
  antisymmetrize,
  symmetricQ,
  antisymmetricQ,
  zeroQ,
  inverse,
  contract,
  raise,
  lower,
  norm,
  partial,
  dg,
  Christoffel,
  divergence,
  scalarLaplacian,
  covariant,
  up,
  down,
  none,
  Lie,
  commutator,
  zeroTensor,
  rg,
  LeviCivita,
  Riemann,
  RicciTensor,
  RicciScalar,
  Einstein,
  Weyl,
  Cotton,
  dd,
  \[DoubleStruckD],
  ranks,
  Wedge,
  wedge,
  WedgePower,
  WedgeExp,
  exterior,
  Lietemp,
  volumeForm,
  HodgeStar,
  formContract,
  FormToTensor,
  TensorToForm,
  hypersurface,
  unitnormal,
  projector,
  extrinsic,
  extrinsictrace,
  induced 
  ];

Protect @@ coord;

(*

changelog (items marked with * affect users; items not marked with * are internal):

June 2015:
(1) fixed bug in divergence, which (despite the documentation) gave the wrong answer when applied to an antisymmetric tensor of rank higher than 1 (thanks to A. Seraj for pointing this out)

February 2013:
(1)* renamed Laplacian to scalarLaplacian, to avoid conflict with new built-in function Laplacian in Mathematica 9

June 2010:
(1)* added norm

February 2010:
(1)* made display work, after hypersurface is called, with tensors (such as induced) whose dimensionality at every level equals dimen-1
(2)* added formContract
(3)* made Lie work on dd expressions
(4)* added HScoord to hypersurface
(4) added FullTensorQ and restricted functions display, rank, covariant (when called without index positions specified), Wedge, HodgeStar, TensorToForm so they only work on expressions for which FullTensorQ returns True (with exception for display explained in item (1) above)
(5) changed Part and Extract so they only work with coordinate names when the length of the tensor at the corresponding level equals dimen (or, after hypersurface is called, dimen-1)

December 2009:
(1) changed the code for coordQ and Extract to fix a bug when using DSolve

October 2009:
(1)* added functions swapIndices, symmetricQ, antisymmetricQ
(2)* added Cotton
(2) reverted the code for partial to its previous form, because of a report that the new form was causing problems with Series objects (presumably due to a bug in Mathematica, but I have not tracked the problem down in detail)

September 2009:
(1)* added support for representing differential forms as algebraic expressions in terms of a basis of differential forms (rather than an array); the basis is expressions like dd[t], dd[t,r], etc; functions HodgeStar, wedge, Wedge, exterior, rank work with these expressions; added new functions ranks, TensorToForm, FormToTensor, WedgeExp, WedgePower, volumeForm
(2)* rewrote Part to work with more general part specifications, like All, {t,r}, etc., and to let such a part specification appear on the LHS of an assignment
(3)* made Extract work with coordinate names, e.g. Extract[ Riemann, {t,r,t,r} ]
(4)* added function zeroTensor
(5)* made (hopefully) all functions work properly with SparseArray objects (that being said, it is strongly recommended that SparseArray objects NOT be used, due to a bug in Outer that can lead to incorrect results for outer products of tensors! (Note added 11/23/10: initial testing appears to show that this bug has been fixed in Version 8)
(6)* made program abort if the metric is not kosher
(7)* added function commutator
(8)* allowed covariant to accept "none" as an index position
(9) added function coordQ and changed NameToNumber
(10) added function scalarQ
(11) added protection for functions called by hypersurface

August 2009:
(1)* added changelog
(2)* changed HodgeStar to agree with Wald's definition
(3) added protection for internal variables and functions dimen, dg, Chrfel, padindexlist, NameToNumber
(4) eliminated redundant Clear at the beginning
(5) replaced Module with more appropriate With throughout

July 2009:
(1)* added Wedge
(2)* made exterior, Wedge, **, symetrize, antisymmetrize, HodgeStar behave correctly with scalars
(3)* changed contract so it only takes index pairs rather than longer lists (contract[ tensor, {index1,index2},{index3,index4} ] rather than contract[ tensor, {index1,index2,index3,index4} ])
(4)* allowed tr, symmetrize, antisymmetrize take multiple lists of indices
(5)* made many of the functions more picky about what type of argument they would take (to avoid accidental misuse)
(6)* made tr and contract by default act on the first two indices
(7) simplified programming of Lie and covariant

May 2009:
(1)* added function display
(2)* added unprotect of various functions and clearing of various definitions at the beginning of the package, to let users re-load the package without re-starting the kernel
(3) probably some other small changes I've forgotten now

*)