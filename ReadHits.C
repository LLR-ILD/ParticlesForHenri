
//*C  S  M  I  J    T  L  Q  E      x        y        z       nnel nel  pdg t*
// a  t  o  c  c    o  a  h  n                                !    e     t  i
// l  a  d  e  e    w  y  a  e                                e    l     y  m
// o  v  u  l  l    e  e  r  r                                l    e     p  e
//    e  l  l  l    r  r  g  g                                e    c     e
//       e          e  y                                      c
auto T = new TNtuple("h", "hits", "C:S:M:I:J:T:L:Q:E:x:y:z:nnel:nel:pdg:t");
T->SetAlias("r","sqrt(x*x+y*y)")

// To have one big file run in shell
// for f in data/v5_big/mu-/py_ascii/*.hits; cat $f >> /tmp/all.hits
.! for f in data/v5_big/mu-/py_ascii/*.hits; cat $f >> /tmp/all.hits
T->ReadFile("/tmp/all.hits");
