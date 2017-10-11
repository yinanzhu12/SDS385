// [[Rcpp::depends(RcppEigen)]]
#include <Rcpp.h>
#include <RcppEigen.h>
#include <vector>
using namespace Rcpp;
using namespace std;
typedef Eigen::MappedSparseMatrix<double> MSpMat;
typedef MSpMat::InnerIterator InIterMat;
typedef NumericVector NV;
typedef IntegerVector IV;
typedef double db;
typedef vector<int> vi;
typedef vector<double> vdb;
#define sz(a) int((a).size()) 
#define lop(i,a,b) for (int i=a; i<=b; i++)
#define vlop(i,v) lop(i,0,sz(v)-1)
#define pb push_back
//[[Rcpp::export]]
db targetfunction(MSpMat X, NV y, NV beta,db lambda){
  db ret=0;
  lop(i,0,X.cols()-1){
    db og=0;
    for(InIterMat it(X,i);it;++it)og-=it.value()*beta[it.row()];
    og=1/(exp(og)+1);
    if(y[i])ret-=log(og);
    else ret-=log(1-og);
  }
  if(lambda){
    lop(i,0,X.rows()-1)ret+=lambda*abs(beta[i]);
  }
  return ret;
}

//[[Rcpp::export]]
NV sgdC(NV rn, MSpMat X,NV y,int epoch){
  int p=X.rows(),r,ite=X.cols()*epoch;
  vdb H(p,0.001);
  NV beta(p),ret(p+epoch);
  db og,g;
  lop(i,0,ite-1){
    og=0;
    r=rn[i%X.cols()];
    for(InIterMat it(X,r); it;++it)og-=it.value()*beta[it.row()];
    og=1/(exp(og)+1);
    for(InIterMat it(X,r); it;++it){
      g=(og-y[r])*it.value();
      int j=it.row();
      H[j]+=g*g;
      beta[j]-=g/sqrt(H[j]);
    }
    if((i+1)%(X.cols())==0)ret[(i+1)/X.cols()-1]=targetfunction(X,y,beta,0);
  }
  lop(i,0,p-1)ret[epoch+i]=beta[i];
  return ret;
}
//[[Rcpp::export]]
db lazyupdate(db beta,int betaindex,vdb& H, vi& lastupdate, db lambda,int current){
  if(beta==0)return 0;
  if(current-lastupdate[betaindex]<=1)return beta;
  if(current-lastupdate[betaindex]==2){
    H[betaindex]+=lambda*lambda;
    lastupdate[betaindex]++;
    if(beta>0)return beta-lambda/sqrt(H[betaindex]);
    return beta+lambda/sqrt(H[betaindex]);
  }
  int n=current-lastupdate[betaindex]-1;
  lastupdate[betaindex]=current-1;
  db change=2/lambda*(sqrt(H[betaindex]+lambda*lambda*n)-sqrt(H[betaindex]+lambda*lambda));
  if(abs(beta)<change)return 0;
  H[betaindex]+=lambda*lambda*n;
  if(beta>0)return beta-change;
  return beta+change;
}

//[[Rcpp::export]]
NV sgdC_lasso(NV rn, MSpMat X,NV y,int epoch,db lambda){
  int p=X.rows(),r,ite=X.cols()*epoch;
  vdb H(p,0.001);
  NV ret(p+epoch),beta(p);
  vi lastupdate(p,0);
  db og,g;
  lop(i,0,ite-1){
    og=0;
    r=rn[i%X.cols()];
    for(InIterMat it(X,r); it;++it){
      int j=it.row();
      beta[j]=lazyupdate(beta[j],j,H,lastupdate,lambda,i);
      og-=it.value()*beta[j];
    }
    og=1/(exp(og)+1);
    for(InIterMat it(X,r); it;++it){
      int j=it.row();
      g=(og-y[r])*it.value();
      if(beta[j]>0)g+=lambda;
      else if(beta[j]<0)g-=lambda;
      H[j]+=g*g;
      beta[j]-=g/sqrt(H[j]);
      lastupdate[j]=i;
    }
    if((i+1)%(X.cols())==0){
      lop(j,0,p-1)beta[j]=lazyupdate(beta[j],j,H,lastupdate,lambda,i+1);
      ret[(i+1)/X.cols()-1]=targetfunction(X,y,beta,lambda);
    }
  }
  lop(i,0,p-1)ret[epoch+i]=beta[i];
  return ret;
}
