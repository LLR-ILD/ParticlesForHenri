
T->SetMarkerSize(0.2)

// Scatter plot of r vs z, with M number as color
T->Draw("r:z:M", "", "col")
gPad->SaveAs("geom-M-rz.png")

// Scatter Plot of x:y, with Stave number as color 
T->Draw("y:x:S", "M==0 && C==29", "colz")
TLine *line = new TLine(371.9745,2356.164,371.9745,-438.3562);
line->Draw();
line = new TLine(-392.3567,405.4794,-392.3567,-2389.041);
line->Draw();
line = new TLine(-2328.662,405.4794,382.1656,405.4794);
line->Draw();
line = new TLine(-402.5478,-405.4795,2308.28,-405.4795);
line->Draw();
gPad->Modified()
gPad->SaveAs("geom-S-xy-Endcap.png")
gPad->SaveAs("geom-S-xy-Endcap.C")
gPad->SaveAs("geom-S-xy-Endcap.root")

//Towers in the barrel
T->Draw("r:z:T", "C==20", "colz")

// Staves in the Barrel
T->Draw("x:y:S", "C==20", "colz")
gPad->SaveAs("geom-S-xy-Barrel.png")

// Layers in the barrel radial vue
T->Draw("x:y:L", "C==20", "colz")
gPad->SaveAs("geom-L-xy-Barrel.png")

// Layers in polar view
T->Draw("y:z:L", "C==20 && S==3", "colz")
T->Draw("y:z:L", "S%4==3", "colz"); // for staves==3 or 7.

// Towers in polar view
T->Draw("y:z:T", "S%4==3", "colz")
gPad->SaveAs("geom-T-yz-S3.png")
