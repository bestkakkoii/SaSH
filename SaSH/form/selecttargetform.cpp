#include "stdafx.h"
#include "selecttargetform.h"

SelectTargetForm::SelectTargetForm(QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SelectTargetForm::~SelectTargetForm()
{
}

void SelectTargetForm::showEvent(QShowEvent* e)
{
	setAttribute(Qt::WA_Mapped);
	QDialog::showEvent(e);
}
